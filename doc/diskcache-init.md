## (Re)Init disk cache

A. Dispose


B. Reload

1. Check directory `tables`. If not exist, then create.

2. Check file `tables/data.bin`. If not exist, create a FragmentTable and save into the file. If exist, then load into a FragmentTable.

3. Ask RegionManager to load configured regions

