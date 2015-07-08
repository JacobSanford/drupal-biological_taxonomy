<?php
/**
 * @file
 * Batch API functionality for adding indices to an ITIS database file.
 */

require_once drupal_get_path('module', 'biological_taxonomy') .
  '/includes/itis/db.inc';

/**
 * Batch callback: Generate batch definition for adding indices to ITIS DB.
 *
 * @return object
 *   The batch object to be processed.
 *
 * @see _biological_taxonomy_batch_index_operations()
 * @see _biological_taxonomy_batch_index_finished()
 */
function _biological_taxonomy_batch_create_indices() {
  $_SESSION['index_http_request_count'] = 0;

  $batch = array(
    'operations' => _biological_taxonomy_batch_index_operations(),
    'finished' => '_biological_taxonomy_batch_index_finished',
    'title' => t('Creating Supplemental Database Indices'),
    'init_message' => t('Creating additional ITIS database indices.'),
    'progress_message' => t('Created @current out of @total indices.'),
    'error_message' => t('Additional ITIS database index creation has encountered an error.'),
    'file' => drupal_get_path('module', 'biological_taxonomy') . '/includes/batch/indices.inc',

  );
  return $batch;
}

/**
 * Implements callback_batch_operations().
 */
function _biological_taxonomy_batch_index_operations() {
  require_once drupal_get_path('module', 'biological_taxonomy') .
    '/includes/itis/db.inc';

  $operations = array();
  foreach (_biological_taxonomy_get_db_indices() as $cur_index) {
    $operations[] = array(
      '_bt_index_add',
      array(
        $cur_index['table'],
        $cur_index['name'],
        $cur_index['columns'],
      ),
    );
  }
  return $operations;
}

/**
 * Implements callback_batch_finished().
 */
function _biological_taxonomy_batch_index_finished($success, $results, array $operations) {
  if ($success) {
    drupal_set_message(
      t(
        '@requests indices were added to the ITIS database.',
        array(
          '@requests' => _biological_taxonomy_batch_index_get_requests(),
        )
      )
    );
  }
  else {
    $error_operation = reset($operations);
    drupal_set_message(
      t('An error occurred while processing @operation with arguments : @args',
        array(
          '@operation' => $error_operation[0],
          '@args' => print_r($error_operation[0], TRUE),
        )
      )
    );
  }
}

/**
 * Batch Operation callback: Create a single index in the ITIS database.
 *
 * @param string $table_name
 *   The name of the table to create the index within.
 * @param string $index_name
 *   The name of the index to create.
 * @param array $columns
 *   An ordered array containing the columns to create the index on.
 * @param array $context
 *   The current batch import context associative array.
 */
function _biological_taxonomy_index_add($table_name, $index_name, array $columns, &$context) {
  require_once drupal_get_path('module', 'biological_taxonomy') .
    '/includes/itis/db.inc';

  $itis_dbh = _biological_taxonomy_get_dbh();
  _biological_taxonomy_create_index($itis_dbh, $table_name, $index_name, $columns);

  $context['message'] = t('Created index "@indexname"', array('@indexname' => $index_name));
  _biological_taxonomy_batch_index_update_requests();
}


/**
 * Increment HTTP requests in a session variable.
 */
function _biological_taxonomy_batch_index_update_requests() {
  $_SESSION['index_http_request_count']++;
}

/**
 * Return HTTP requests value from a session variable.
 *
 * @return int
 *   Value of session variable 'index_http_request_count'.
 */
function _biological_taxonomy_batch_index_get_requests() {
  return !empty($_SESSION['index_http_request_count']) ? $_SESSION['index_http_request_count'] : 0;
}
