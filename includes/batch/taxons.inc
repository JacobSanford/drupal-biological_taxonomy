<?php
/**
 * @file
 * Batch API functionality for importing a vocabulary from an ITIS database.
 */

require_once drupal_get_path('module', 'biological_taxonomy') .
  '/includes/itis/db.inc';

/**
 * Batch callback: Generate definition to import a vocabulary from an ITIS DB.
 *
 * @param int $kingdom_id_selected
 *   The ITIS kingdom TSN ID.
 * @param int $bottom_limit_selected
 *   The rank_id of the bottom limit to descend in populating the taxonomy.
 * @param int $only_valid_accepted_selected
 *   1/0 Only include valid/accepted taxons?.
 * @param string $vocabulary_name
 *   Name of the vocabulary to create.
 * @param string $vocabulary_machine_name
 *   Drupal machine name of the vocabulary to create.
 * @param int $number_taxons
 *   Number of taxons that are being imported.
 *
 * @return object
 *   The batch object to be processed.
 *
 * @see _biological_taxonomy_batch_taxon_operations()
 * @see _biological_taxonomy_batch_taxon_finished()
 */
function _biological_taxonomy_batch_import_taxons($kingdom_id_selected, $bottom_limit_selected, $only_valid_accepted_selected, $vocabulary_name, $vocabulary_machine_name, $number_taxons) {
  $t = get_t();

  $new_tax_vocab = _biological_taxonomy_create_vocabulary($vocabulary_name, $vocabulary_machine_name);

  drupal_set_message(
    $t('Created a new vocabulary (@macnam:@vid)',
      array(
        '@macnam' => $vocabulary_machine_name,
        '@vid' => $new_tax_vocab->vid,
      )
    )
  );
  variable_set('biological_taxonomy_current_import_vid', $new_tax_vocab->vid);
  variable_set('biological_taxonomy_current_import_vocab_name', $vocabulary_machine_name);

  $_SESSION['taxon_http_request_count'] = 0;
  $batch = array(
    'operations' => _biological_taxonomy_batch_taxon_operations(
      $kingdom_id_selected,
      $bottom_limit_selected,
      $only_valid_accepted_selected
    ),
    'finished' => '_biological_taxonomy_batch_taxon_finished',
    'title' => $t('Creating taxonomy Terms'),
    'init_message' => $t('Creating a structured taxonomy from the ITIS data.'),
    'progress_message' => $t('Created @current out of @total terms.'),
    'error_message' => $t('Biological taxonomy creation has encountered an error.'),
    'file' => drupal_get_path('module', 'biological_taxonomy') . '/includes/batch/taxons.inc',
  );

  return $batch;
}

/**
 * Implements callback_batch_operations().
 *
 * @param int $kingdom_id_selected
 *   The ITIS kingdom TSN ID.
 * @param int $bottom_limit_selected
 *   The rank_id of the bottom limit to descend in populating the taxonomy.
 * @param int $only_valid_accepted_selected
 *   1/0 Only include valid/accepted taxons?.
 */
function _biological_taxonomy_batch_taxon_operations($kingdom_id_selected, $bottom_limit_selected, $only_valid_accepted_selected) {
  $operations = array();

  $itis_dbh = _biological_taxonomy_get_dbh();
  $kingdom_term = _biological_taxonomy_get_tax_term($itis_dbh, $kingdom_id_selected);
  $kingdom_tinyint_id = _biological_taxonomy_get_kingdom_id(
    $itis_dbh,
    $kingdom_term['complete_name']
  );
  $filter_valid_accepted = $only_valid_accepted_selected == 1 ?
    'AND (n_usage="accepted" OR n_usage="valid")' :
    NULL;
  $get_kingdom_id_query = "SELECT tsn FROM taxonomic_units WHERE kingdom_id = '$kingdom_tinyint_id' AND rank_id<='$bottom_limit_selected' $filter_valid_accepted ORDER BY rank_id ASC";
  foreach ($itis_dbh->query($get_kingdom_id_query) as $row) {
    $operations[] = array(
      '_bt_4dd',
      array(
        $row['tsn'],
      ),
    );
  }
  return $operations;
}

/**
 * Implements callback_batch_finished().
 */
function _biological_taxonomy_batch_taxon_finished($success, $results, array $operations) {
  $t = get_t();
  $vocab_machine_name = variable_get('biological_taxonomy_current_import_vocab_name');

  variable_del('biological_taxonomy_current_import_vid');
  variable_del('biological_taxonomy_current_import_vocab_name');

  if ($success) {
    drupal_set_message(
      $t('@requests taxonomy terms were built into a taxonomy tree.',
        array(
          '@requests' => _biological_taxonomy_batch_taxon_get_requests(),
        )
      )
    );
    drupal_goto('admin/structure/taxonomy/' . $vocab_machine_name);
  }
  else {
    $error_operation = reset($operations);
    drupal_set_message(
      $t('An error occurred while processing @operation with arguments : @args',
        array(
          '@operation' => $error_operation[0],
          '@args' => print_r($error_operation[0], TRUE),
        )
      )
    );
  }
}

/**
 * Batch Operation Callback : Create taxonomy term.
 *
 * The reason for the short and cryptic function name is to save memory in the
 * potentially huge operations array that a large taxonomy import would
 * generate. Here be dragons.
 */
function _bt_4dd($tsn, &$context) {
  $t = get_t();

  $itis_dbh = _biological_taxonomy_get_dbh();
  $cur_term = _biological_taxonomy_get_tax_term($itis_dbh, $tsn);
  $vid = variable_get('biological_taxonomy_current_import_vid', NULL);
  $parent_term_id = _biological_taxonomy_get_tid_from_tsn($cur_term['parent_tsn'], $vid);

  $term_data = array(
    'name' => $cur_term['complete_name'],
    'description' => $cur_term['complete_name'],
    'parent' => $parent_term_id,
    'vocabulary_machine_name' => variable_get(
      'biological_taxonomy_current_import_vocab_name',
      ''
    ),
  );

  $term_entity = entity_create('taxonomy_term', $term_data);
  $term_wrapper = entity_metadata_wrapper('taxonomy_term', $term_entity);
  $term_wrapper->vocabulary->set($vid);
  $term_wrapper->tax_imported_tsn->set((int) $tsn);
  $term_wrapper->save();

  $context['message'] = $t(
    'Created taxonomy term "@title"',
    array(
      '@title' => $cur_term['complete_name'],
    )
  );

  _biological_taxonomy_batch_taxon_update_requests();
}

/**
 * Increment HTTP requests in a session variable.
 */
function _biological_taxonomy_batch_taxon_update_requests() {
  $_SESSION['taxon_http_request_count']++;
}

/**
 * Return HTTP requests value from a session variable.
 *
 * @return int
 *   Value of session variable 'taxon_http_request_count'.
 */
function _biological_taxonomy_batch_taxon_get_requests() {
  return !empty($_SESSION['taxon_http_request_count']) ?
    (int) $_SESSION['taxon_http_request_count'] :
    0;
}
