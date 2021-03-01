db = "/path/to/dataset.silo"
OpenDatabase(db)

AddPlot("Pseudocolor", "helium")
AddOperator("Resample")
ra = ResampleAttributes()
ra.samplesX = 512
ra.samplesY = 512
ra.samplesZ = 512
ra.distributedResample = False
SetOperatorOptions(ra)

DrawPlots()
dbAtts = ExportDBAttributes()
dbAtts.db_type = "BOV"
dbAtts.filename = "singlehelium"
dbAtts.variables = ("helium")
ExportDatabase(dbAtts)

RemoveAllOperators()
DeleteAllPlots()

CloseDatabase(db)

import sys
sys.exit(0)
