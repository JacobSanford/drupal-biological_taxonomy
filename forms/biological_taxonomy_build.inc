<?php

/**
 * @file
 * Builds the biological taxonomy admin configuration form.
 */

/**
 * Form constructor for the biological taxonomy build options.
 *
 * @ingroup forms
 *
 * @see biological_taxonomy_build_form_submit()
 */
function biological_taxonomy_build_form($form, &$form_state) {
  require_once drupal_get_path('module', 'biological_taxonomy') .
    '/includes/itis/db.inc';

  if (empty($form)) {
    $form = array();
  }
  $itis_dbh = _biological_taxonomy_get_dbh();

  $options_top_level_kingdom = _biological_taxonomy_get_kingdom_list($itis_dbh);
  $kingdom_selected = isset($form_state['values']['biological_taxonomy_top_level_kingdom']) ?
    $form_state['values']['biological_taxonomy_top_level_kingdom'] :
    key($options_top_level_kingdom);

  $form['biological_taxonomy_top_level_kingdom'] = array(
    '#type' => 'select',
    '#title' => t('Top Level Kingdom'),
    '#options' => $options_top_level_kingdom,
    '#default_value' => $kingdom_selected,
    '#description' => t('Select the kingdom to serve as the top level taxon in the new taxonomy.'),
    '#required' => TRUE,
    '#ajax' => array(
      'callback' => '_biological_taxonomy_update_limit_num_items',
    ),
  );

  $options_taxonomy_bottom_limit = _biological_taxonomy_get_subrank_list(
    $itis_dbh,
    $options_top_level_kingdom[$kingdom_selected]
  );
  $bottom_limit_selected = isset($form_state['values']['biological_taxonomy_bottom_limit']) ?
    $form_state['values']['biological_taxonomy_bottom_limit'] :
    key($options_taxonomy_bottom_limit);

  $form['biological_taxonomy_bottom_limit'] = array(
    '#type' => 'select',
    '#title' => t('Stop Populating Taxonomy At:'),
    '#prefix' => '<div id="bottom-limit-wrapper">',
    '#suffix' => '</div>',
    '#options' => $options_taxonomy_bottom_limit,
    '#default_value' => $bottom_limit_selected,
    '#description' => t('Select the rank level below which the taxonomy tree will not be populated.'),
    '#required' => TRUE,
    '#ajax' => array(
      'callback' => '_biological_taxonomy_update_bottom_limit',
    ),
  );

  $only_valid_accepted_selected = isset($form_state['values']['biological_taxonomy_limit_valid_accepted']) ?
    $form_state['values']['biological_taxonomy_limit_valid_accepted'] :
    1;

  $form['biological_taxonomy_limit_valid_accepted'] = array(
    '#type' => 'checkbox',
    '#title' => t('Only include valid/accepted taxons'),
    '#prefix' => '<div id="valid-accepted-wrapper">',
    '#suffix' => '</div>',
    '#default_value' => $only_valid_accepted_selected,
    '#ajax' => array(
      'callback' => '_biological_taxonomy_update_number_items',
    ),
  );

  $num_items_to_import = _biological_taxonomy_get_num_taxons(
    $itis_dbh,
    $options_top_level_kingdom[$kingdom_selected],
    $bottom_limit_selected,
    $only_valid_accepted_selected
  );

  $form['biological_taxonomy_number_taxons_label'] = array(
    '#prefix' => '<div id="number-items-label-wrapper"><p>',
    '#suffix' => '</p></div>',
    '#markup' => t('With this configuration, approximately @numitems taxon classifications will be imported.',
      array(
        '@numitems' => number_format(floatval($num_items_to_import)),
      )
    ),
  );

  $form['biological_taxonomy_number_taxons'] = array(
    '#prefix' => '<div id="number-items-wrapper">',
    '#suffix' => '</div>',
    '#type' => 'hidden',
    '#value' => $num_items_to_import,
  );

  $taxonomy_name_value = isset($form_state['values']['biological_taxonomy_name']) ?
    $form_state['values']['biological_taxonomy_name'] :
    '';
  $form['biological_taxonomy_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name of Taxonomy to Create'),
    '#default_value' => $taxonomy_name_value,
    '#maxlength' => 255,
    '#required' => TRUE,
  );

  $taxonomy_machine_name_value = isset($form_state['values']['biological_taxonomy_machine_name']) ?
    $form_state['values']['biological_taxonomy_machine_name'] :
    '';
  $form['biological_taxonomy_machine_name'] = array(
    '#type' => 'machine_name',
    '#default_value' => $taxonomy_machine_name_value,
    '#maxlength' => 255,
    '#machine_name' => array(
      'exists' => 'taxonomy_vocabulary_machine_name_load',
    ),
  );

  $form['biological_taxonomy_build_submit'] = array(
    '#type' => 'submit',
    '#value' => t('Build Taxonomy from ITIS Data'),
  );

  $form['#submit'][] = 'biological_taxonomy_build_form_submit';

  return $form;
}

/**
 * Form submission handler for biological_taxonomy_build_form().
 */
function biological_taxonomy_build_form_submit($form, &$form_state) {
  require_once drupal_get_path('module', 'biological_taxonomy') .
    '/includes/batch/taxons.inc';

  batch_set(
    _biological_taxonomy_batch_import_taxons(
      $form_state['values']['biological_taxonomy_top_level_kingdom'],
      $form_state['values']['biological_taxonomy_bottom_limit'],
      $form_state['values']['biological_taxonomy_limit_valid_accepted'],
      $form_state['values']['biological_taxonomy_name'],
      $form_state['values']['biological_taxonomy_machine_name'],
      $form_state['values']['biological_taxonomy_number_taxons']
    )
  );
}

/**
 * Page callback: Renders the admin taxonomy import page.
 *
 * @return string
 *   Rendered HTML to display in the admin menu page.
 *
 * @see hook_menu()
 */
function _biological_taxonomy_render_admin_page() {
  require_once drupal_get_path('module', 'biological_taxonomy') .
    '/includes/itis/db.inc';

  $library = libraries_detect('itis');
  if (empty($library['error'])) {
    $itis_dbh = _biological_taxonomy_get_dbh();
    if (!_biological_taxonomy_verify_indices($itis_dbh)) {
      require_once drupal_get_path('module', 'biological_taxonomy') .
        '/includes/batch/indices.inc';

      $_SESSION['http_request_count'] = 0;

      batch_set(
        _biological_taxonomy_batch_create_indices()
      );
      batch_process('admin/config/biological_taxonomy/build');
    }
    else {
      $biological_taxonomy_build_form = drupal_get_form('biological_taxonomy_build_form');
      return drupal_render($biological_taxonomy_build_form);
    }
  }
  else {
    drupal_set_message(t('ERROR : ITIS Database not valid.'), 'error');
    return '<p>There is a problem with the ITIS database file. See the ' .
    l(t('status reports page'), 'admin/reports/status') .
    ' for more information.';
  }
  return FALSE;
}

/**
 * AJAX callback: Returns "Number of Taxon" elements to re-render.
 *
 * @see ajax.inc
 */
function _biological_taxonomy_update_number_items($form, $form_state) {
  return array(
    '#type' => 'ajax',
    '#commands' => array(
      ajax_command_replace(
        '#number-items-label-wrapper',
        render($form['biological_taxonomy_number_taxons_label'])
      ),
      ajax_command_replace(
        '#number-items-wrapper',
        render($form['biological_taxonomy_number_taxons'])
      ),
    ),
  );
}

/**
 * AJAX callback: Returns Num Taxon and bottom Limit elements to re-render.
 *
 * @see ajax.inc
 */
function _biological_taxonomy_update_limit_num_items($form, $form_state) {
  return array(
    '#type' => 'ajax',
    '#commands' => array(
      ajax_command_replace(
        '#bottom-limit-wrapper',
        render($form['biological_taxonomy_bottom_limit'])
      ),
      ajax_command_replace(
        '#number-items-label-wrapper',
        render($form['biological_taxonomy_number_taxons_label'])
      ),
      ajax_command_replace(
        '#number-items-wrapper',
        render($form['biological_taxonomy_number_taxons'])
      ),
    ),
  );
}

/**
 * AJAX callback: Returns "Bottom Limit" elements to re-render.
 *
 * @see ajax.inc
 */
function _biological_taxonomy_update_bottom_limit($form, $form_state) {
  return array(
    '#type' => 'ajax',
    '#commands' => array(
      ajax_command_replace('#number-items-label-wrapper',
        render($form['biological_taxonomy_number_taxons_label'])
      ),
      ajax_command_replace('#number-items-wrapper',
        render($form['biological_taxonomy_number_taxons'])
      ),
    ),
  );
}
