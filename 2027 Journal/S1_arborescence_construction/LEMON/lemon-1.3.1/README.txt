Differences compared to LEMON stock version 1.3.1 dowloaded at 2026.07.22 from https://lemon.cs.elte.hu/trac/lemon/wiki/Downloads

1) In the lemon/ folder:
- modified path.h to compile with clang on MacOS:
        // WARNING: modified based on https://stackoverflow.com/questions/33632584/no-viable-conversion-with-lemon-for-clang-but-valid-for-g to complie on MaxOS
        //  head.push_back(it);
        head.push_back(it.operator const typename CPath::Arc());
- added gurobi.h and gurobi.cc for interface with the Gurobi solver together with a modified version of lp.h
- modified arg_parser.h by making some private variables public to make it compatible with lemonRouting/
- added config.h from a previous install.
