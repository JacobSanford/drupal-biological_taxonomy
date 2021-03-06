Drupal Biological Taxonomy
=============

[![Build Status](https://travis-ci.org/JacobSanford/drupal-biological_taxonomy.svg)](https://travis-ci.org/JacobSanford/drupal-biological_taxonomy)

This module provides functionality to build a tree based biological taxonomy
sourced from the ITIS database (http://www.itis.gov/downloads/). It leverages:

* Batch API
* Libraries
* Entity

to import each element as a taxonomy term.

Depending on the 'cutoff' level selected, generating the taxonomy could take a
long time. Be aware that extremely large taxonomies can have adverse effects on
Drupal.

As with all hierarchical taxonomy trees, a module such as taxonomy_manager is
recommended for use in conjunction with this one.

Installation
-----------
This module requires PDO SQLite (3.x or above) be installed.

Before enabling, unzip and place the ITIS.sqlite and file into /sites/all/itis/.

New contributions/issue reports welcomed.

License
-----------
- Drupal Biological Taxonomy is licensed under the MIT License:
  - http://opensource.org/licenses/mit-license.html
- Attribution is not required, but much appreciated:
  - `Biological Taxonomy Drupal module by Jacob Sanford`
