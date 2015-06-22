<?php

/**
 * @file
 * Provides functionality to create indices in the ITIS database.
 */

/**
 * Defines the indices to create in the ITIS database.
 *
 * @return array
 *   An associative array describing the SQLite indices.
 */
function _biological_taxonomy_get_db_indices() {
  return array(
    array(
      'name' => 'IDX_TAX_TYPES_RANK_KINGDOM_USAGE',
      'table' => 'taxon_unit_types',
      'columns' => array(
        'rank_id',
        'kingdom_id',
      ),
    ),
    array(
      'name' => 'IDX_TAX_UNITS_PARENT_USAGE',
      'table' => 'taxonomic_units',
      'columns' => array(
        'parent_tsn',
        'n_usage',
      ),
    ),
    array(
      'name' => 'IDX_TAX_UNITS_PARENT_TSN',
      'table' => 'taxonomic_units',
      'columns' => array(
        'parent_tsn',
      ),
    ),
  );
}

/**
 * Verify that the module required indices exist in the SQLite database.
 *
 * @param object $itis_dbh
 *   A PDO SQLite database handle.
 *
 * @return bool
 *   A boolean indicating whether the indices exist.
 */
function _biological_taxonomy_verify_indices($itis_dbh) {
  foreach (_biological_taxonomy_get_db_indices() as $db_index) {
    if (_biological_taxonomy_table_has_index($itis_dbh, $db_index['table'], $db_index['name']) === FALSE) {
      return FALSE;
    }
  }
  return TRUE;
}

/**
 * Create an index in a SQLite database.
 *
 * @param object $itis_dbh
 *   A PDO SQLite database handle.
 * @param string $table_name
 *   The name of the table to create the index in.
 * @param string $index_name
 *   The name of the index.
 * @param array $index_columns
 *   An ordered array containing the columns to create the index on.
 *
 * @return bool
 *   A boolean indicating whether the index creation was successful.
 */
function _biological_taxonomy_create_index($itis_dbh, $table_name, $index_name, array $index_columns) {
  $columns = implode(',', $index_columns);
  $sth = $itis_dbh->prepare("CREATE INDEX IF NOT EXISTS $index_name ON $table_name ($columns)");
  return $sth->execute();
}

/**
 * Verify that an index exists in a SQLite table.
 *
 * @param object $itis_dbh
 *   A PDO SQLite database handle.
 * @param string $table_name
 *   The name of the table to check for the index in.
 * @param string $index_name
 *   The name of the index to check for.
 *
 * @return bool
 *   A boolean indicating whether the index exists.
 */
function _biological_taxonomy_table_has_index($itis_dbh, $table_name, $index_name) {
  $sth = $itis_dbh->prepare("pragma index_list($table_name)");
  $sth->execute();
  return _biological_taxonomy_index_exists($index_name, $sth->fetchAll());
}

/**
 * Check a SQLite index result set for an index.
 *
 * @param string $index_name
 *   The name of the index to check for.
 * @param array $table_indices
 *   An associative array containing the SQLite result of querying the indices.
 *
 * @return bool
 *   A boolean indicating whether the index exists within the result.
 */
function _biological_taxonomy_index_exists($index_name, array $table_indices) {
  foreach ($table_indices as $cur_index) {
    if (isset($cur_index['name']) && $cur_index['name'] == $index_name) {
      return TRUE;
    }
  }
  return FALSE;
}
