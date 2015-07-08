<?php
/**
 * @file
 * Provides functionality to translate an ITIS SQLite3 DB to a vocabulary.
 */

require_once 'indices.inc';

/**
 * Create a new vocabulary with required attached fields.
 *
 * @param string $vocabulary_name
 *   Name of the vocabulary to create.
 * @param string $vocabulary_machine_name
 *   Drupal machine name of the vocabulary to create.
 *
 * @return object
 *   The vocabulary object that was created.
 */
function _biological_taxonomy_create_vocabulary($vocabulary_name, $vocabulary_machine_name) {
  $vocabulary = array(
    'name' => $vocabulary_name,
    'description' => t('Biological Taxonomy Imported from ITIS Database'),
    'machine_name' => $vocabulary_machine_name,
  );

  $vocabulary_entity = entity_create('taxonomy_vocabulary', $vocabulary);
  $vocabulary_wrapper = entity_metadata_wrapper('taxonomy_vocabulary', $vocabulary_entity);
  $vocabulary_wrapper->save();

  $new_tax_instances = array(
    'tax_imported_tsn' => array(
      'label' => t('TSN from ITIS Database'),
      'field_name' => 'tax_imported_tsn',
      'widget' => array(
        'type' => 'text_textfield',
      ),
      'entity_type' => 'taxonomy_term',
      'bundle' => $vocabulary_machine_name,
    ),
  );

  foreach ($new_tax_instances as $instance) {
    field_create_instance($instance);
  }

  return $vocabulary_entity;
}

/**
 * Get ITIS taxonomic term corresponding to a TSN value.
 *
 * @param object $itis_dbh
 *   A PDO SQLite database handle.
 * @param int $tsn
 *   The TSN id to return the term for.
 *
 * @return array
 *   An associative array containing taxonomic data.
 */
function _biological_taxonomy_get_tax_term($itis_dbh, $tsn) {
  $get_term_query = "SELECT tsn, complete_name, parent_tsn, rank_id FROM taxonomic_units WHERE tsn='$tsn'";
  foreach ($itis_dbh->query($get_term_query) as $row) {
    return $row;
  }
  return FALSE;
}

/**
 * Get a TID from a vocabulary corresponding to a TSN value.
 *
 * @param int $tsn
 *   The TSN id to return the term for.
 * @param int $vid
 *   The vocabulary ID to search for the TSN value.
 *
 * @return int
 *   A TID corresponding to the matching TSN, or 0 if not match.
 */
function _biological_taxonomy_get_tid_from_tsn($tsn, $vid) {
  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', 'taxonomy_term')
    ->propertyCondition('vid', $vid)
    ->fieldCondition('tax_imported_tsn', 'value', $tsn, '=');
  $result = $query->execute();
  if (isset($result['taxonomy_term'])) {
    foreach ($result['taxonomy_term'] as $cur_term) {
      return $cur_term->tid;
    }
  }
  return 0;
}

/**
 * Generate a select list compatible array of kingdom and TSN IDs.
 *
 * @param object $itis_dbh
 *   A PDO SQLite database handle.
 *
 * @return array
 *   Form API compatible associative array of kingdoms and TID
 */
function _biological_taxonomy_get_kingdom_list($itis_dbh) {
  $kingdom_list = array();
  $get_kingdoms_query = "SELECT tsn, complete_name FROM taxonomic_units WHERE parent_tsn=0 AND (n_usage='accepted' OR n_usage='valid')";
  foreach ($itis_dbh->query($get_kingdoms_query) as $row) {
    $kingdom_list[$row['tsn']] = $row['complete_name'];
  }
  return $kingdom_list;
}

/**
 * Generate a select list compatible array of lower-rank taxon elements.
 *
 * This is used to determine a low point to stop importing.
 *
 * @param object $itis_dbh
 *   A PDO SQLite database handle.
 * @param string $kingdom_name_selected
 *   The name of the Kingdom serving as the top level element.
 *
 * @return array
 *   Form API compatible associative array of rank ID and rank Name
 */
function _biological_taxonomy_get_subrank_list($itis_dbh, $kingdom_name_selected) {
  $limit_rank_list = array();
  $kingdom_tinyint_id = _biological_taxonomy_get_kingdom_id($itis_dbh, $kingdom_name_selected);
  $get_kingdoms_query = "SELECT rank_id, rank_name FROM taxon_unit_types WHERE kingdom_id='$kingdom_tinyint_id' AND rank_id>10";
  foreach ($itis_dbh->query($get_kingdoms_query) as $row) {
    $limit_rank_list[$row['rank_id']] = $row['rank_name'];
  }
  return $limit_rank_list;
}

/**
 * Gets a SQLite kingdom ID from the name.
 *
 * @param object $itis_dbh
 *   A PDO SQLite database handle.
 * @param string $kingdom_name
 *   The name of the Kingdom.
 *
 * @return int
 *   The kingdom_id as defined in the SQLite Database
 */
function _biological_taxonomy_get_kingdom_id($itis_dbh, $kingdom_name) {
  $get_kingdom_id_query = "SELECT kingdom_id FROM kingdoms WHERE kingdom_name='$kingdom_name'";
  foreach ($itis_dbh->query($get_kingdom_id_query) as $row) {
    return $row['kingdom_id'];
  }
  return FALSE;
}

/**
 * Get the number of taxons that will be imported.
 *
 * @param object $itis_dbh
 *   A PDO SQLite database handle.
 * @param string $kingdom_name_selected
 *   The name of the Kingdom.
 * @param int $bottom_limit_selected
 *   The rank_id of the bottom limit to descend in populating the taxonomy.
 * @param int $only_valid_accepted_selected
 *   Only include valid/accepted taxons?.
 *
 * @return int
 *   The number of entries in the database matching the two parameters.
 */
function _biological_taxonomy_get_num_taxons($itis_dbh, $kingdom_name_selected, $bottom_limit_selected, $only_valid_accepted_selected) {
  $filter_valid_accepted = $only_valid_accepted_selected == 1 ? "AND (n_usage='accepted' OR n_usage='valid')" : NULL;
  $kingdom_tinyint_id = _biological_taxonomy_get_kingdom_id($itis_dbh, $kingdom_name_selected);
  $get_kingdom_id_query = "SELECT COUNT(*) FROM taxonomic_units WHERE kingdom_id = '$kingdom_tinyint_id' AND rank_id<='$bottom_limit_selected' $filter_valid_accepted";
  foreach ($itis_dbh->query($get_kingdom_id_query) as $row) {
    return $row[0];
  }
  return FALSE;
}

/**
 * Retrieve an ITIS PDO SQLite database handle.
 *
 * @return object
 *   Form API compatible associative array of kingdoms and TID
 */
function _biological_taxonomy_get_dbh() {
  $itis_pdo_path = 'sqlite:' .
    variable_get('biological_taxonomy_sqlite_path', DRUPAL_ROOT .
      '/sites/all/libraries/itis/ITIS.sqlite'
    );
  return new PDO($itis_pdo_path);
}

/**
 * Determine if the PDO SQLite driver is available.
 *
 * @return bool
 *   TRUE If the PDO::SQLITE driver is loaded, FALSE Otherwise.
 */
function _biological_taxonomy_pdo_sqlite_exists() {
  try {
    new PDO('sqlite:messaging.sqlite3');
    return TRUE;
  }
  catch (Exception $e) {
    return FALSE;
  }
}

/**
 * Determines if the database has a reasonable structure for ITIS.
 *
 * The function determines if the ITIS SQLite3 Database is valid by checking
 * the 'kingdoms' table for at least 6 entries.
 *
 * This function is used by biological_taxonomy_requirements().
 *
 * @return bool
 *   TRUE If the database contains at least 6 entries in 'kingdoms'.
 *   FALSE Otherwise.
 */
function _biological_taxonomy_check_itis_valid() {
  $itis_dbh = _biological_taxonomy_get_dbh();
  $get_kingdoms_query = "SELECT count(*) FROM kingdoms";

  $query_result = $itis_dbh->query($get_kingdoms_query);
  if (is_object($query_result)) {
    $kingdom_count_result = $query_result->fetch(PDO::FETCH_NUM);
    if ($kingdom_count_result[0] >= 6) {
      return TRUE;
    }
  }
  return FALSE;
}
