#!/bin/bash

set -e

tmp=/InFormant.app

mkdir -p $tmp/Contents/MacOS
mkdir -p $tmp/Contents/Resources

cp /src/dist-res/Info.plist $tmp/Contents
cp /build/in-formant $tmp/Contents/MacOS/InFormant
cp /src/resources/icons/in-formant.icns $tmp/Contents/Resources

sed -i 's/@VERSION@/'"$version"'/g' $tmp/Contents/Info.plist

export OSXCROSS_TARGET_DIR=/osxcross/target

/osxcross/target/bin/macdeployqt $tmp -qmldir=/src/src/qml

# install_name_tool and strip invalidate the signatures, and Apple silicon refuses to run an arm64 file
# with an invalid signature. Sign the bundle again (ad hoc).
rcodesign sign $tmp

srcdmg=${tmp/.app/.dmg}

# Make the disk image here and not with macdeployqt -dmg, because the signing must come first.
# The hfsplus tool copies each symlink as a full file, so the margin is large.
# The unused space compresses to almost nothing in the final .dmg.
size=$(du -b -s $tmp | cut -f1)
truncate -s $((size + 256 * 1048576)) $srcdmg
mkfs.hfsplus -v InFormant $srcdmg
hfsplus $srcdmg mkdir $tmp
hfsplus $srcdmg addall $tmp $tmp

if [ "x$ENABLE_TORCH" = "x1" ]; then
	distdmg=/dist/InFormant-DF-$version-macOS-$TARGET_ARCH.dmg
else
	distdmg=/dist/InFormant-$version-macOS-$TARGET_ARCH.dmg
fi

hfsplus $srcdmg chmod 0755 /InFormant.app/Contents/MacOS/InFormant 

dmg dmg $srcdmg $distdmg
