#!/bin/bash

ssd_parv='./../ssdmodel/valid/Intel_toolkit.parv'
hdd_parv='./../valid/maxtor146g.parv'

ssdout='./sddsim.out'
hddout='./hddsim.out'

cache='c32768_none'

resultDir='./cpff_statistics_dir'     #This dirctoy is used to store some records.

#if there isn't exist this dirctory, then we create it.
if [ ! -d "$resultDir" ]; then
  echo "Create $resultDir"
  mkdir $resultDir
fi

#Workload
# One user
#traceU1=''
#traceU1_2=''
# Multi-user
#traceU2='[u2w1w1]Financial2+WebSearch1_t2000000'
#traceU2_2=''
#traceU2_3=''
#traceU2_4=''

trace='[u2w1w1]Iozone+sql_t200000'

#echo "# Only one user"
#./yusim "./../../yusim_trace/${traceU1}.yt" $ssd_parv $ssdout $hdd_parv $hddout "./output/${traceU1}_${cache}_weighted.stat" "./output/${traceU1}_${cache}_weighted.result"
#./yusim "./../../yusim_trace/${traceU1_2}.yt" $ssd_parv $ssdout $hdd_parv $hddout "./output/${traceU1_2}_${cache}_weighted.stat" "./output/${traceU1_2}_${cache}_weighted.result"


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
./cpff "trace/${trace}.tr" $ssd_parv $ssdout $hdd_parv $hddout