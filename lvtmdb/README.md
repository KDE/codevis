# lvtmdb - Memory database

Provides a(n optionally thread safe) interface to cache database in memory providing
fast lookups.

All the pointers to things named `*Object` (e.g. `lvtmdb::FileObject`) are owned
by the `lvtmdb::ObjectStore` instance.
