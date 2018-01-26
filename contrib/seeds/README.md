# Seeds

Utility to generate the seeds.txt list that is compiled into the client
(see [src/chainparamsseeds.h](/src/chainparamsseeds.h) and other utilities in [contrib/seeds](/contrib/seeds)).

    vi nodes_main.txt
    vi nodes_test.txt
    python3 generate-seeds.py . > ../../src/chainparamsseeds.h
