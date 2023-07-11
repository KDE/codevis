
Package Dependencies
--------------------

 2 bjgc
    |
    v
 1 bjgb

Component Dependencies
----------------------

 bjgb
 ====

 3 bjgb_shoeutil
    |    |   |
    |    |   +---------+
    |    v             |
 2  |  bjgb_shoe       |           bjgb_rules
    |    |   |         |             |    |
    |    |   +---------+    +--------+    |
    |    |             |    |             |
    v    v             v    v             v
 1 bjgb_rank         bjgb_types    bjgb_state     bjgb_dealercount


 bjgc
 ====

 2 bjgc_dealertableutil         bjgc_playertableutil
           |                      |     |
           |    +-----------------+     |
           |    |                       |
           v    v                       v
 1 bjgc_dealertable             bjgc_playertable




