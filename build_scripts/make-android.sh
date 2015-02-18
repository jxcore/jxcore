mkdir out_arm_droid
mkdir out_intel_droid
rm -rf out

mv out_arm_droid out
./android-make-arm.sh $1
mv out out_arm_droid

mv out_intel_droid out
./android-make-intel.sh $1
mv out out_intel_droid
