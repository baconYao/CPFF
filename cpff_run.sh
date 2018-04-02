#!/bin/bash

ssd_parv='./../ssdmodel/valid/Intel_toolkit.parv'
hdd_parv='./../valid/maxtor146g.parv'

ssdout='./sddsim.out'
hddout='./hddsim.out'

#Input the trace here
trace='Financial/F1+F1_t10000000_s100'

#This dirctoy is used to store some records.
resultDir="./cpff_statistics_dir"     

#If there isn't exist this dirctory, then we create it.
if [ ! -d "$resultDir" ]; then
  echo "Create $resultDir"
  mkdir $resultDir
fi

# clean file
echo "Clean old cpff .o file......"
make cpffclean
echo ""
echo ""

# Make cpff file
echo "Make cpff file......."
make
make cpff
echo ""
echo ""


echo "Run cpff......"
./cpff "../../../trace/${trace}.tr" $ssd_parv $ssdout $hdd_parv $hddout