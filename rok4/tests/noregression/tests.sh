NTESTS=0
NERRORS=0

# 1. WMS

# 1.1. GetCapabilitites
let NTESTS+=1
wget  --no-proxy -O tmp.xml "$1?SERVICE=WMS&VERSION=1.1.1&REQUEST=GetCapabilities"

if [ "`xmllint  --schema http://schemas.opengis.net/wms/1.3.0/capabilities_1_3_0.xsd tmp.xml`" ]
then
        echo "Test 1.1. OK"
else
        echo "Test 1.1. FAILED"
        let NERRORS+=1
	echo $NERRORS
fi

rm tmp.xml

# 1.2. GetMap

# 1.2.2 In Jpeg

# 1.2.2.1 Out Rawa
let NTESTS+=1
wget  --no-proxy -O tmp.tif "$1?SERVICE=WMS&VERSION=1.1.1&REQUEST=GetMap&BBOX=805888,6545920,806400,6546432&CRS=IGNF:LAMB93&WIDTH=512&HEIGHT=512&LAYERS=ORTHO_JPEG_TEST&STYLES=&FORMAT=image/tiff&DPI=96&TRANSPARENT=TRUE"
sum="`md5sum tmp.tif | cut -d ' ' -f 1`"
echo $sum
if [ "$sum" = "b70ad3e72e0b66710ca2b93d25f7bfd9" ]
then
	echo "Test 1.2.2.1. OK"
else
	echo "Test 1.2.2.1. FAILED"
        let NERRORS+=1
fi
#rm tmp.tif

# 1.2.2.2. Out Jpeg
let NTESTS+=1
wget  --no-proxy -O tmp.jpg "$1?SERVICE=WMS&VERSION=1.1.1&REQUEST=GetMap&BBOX=805888,6545920,806400,6546432&CRS=IGNF:LAMB93&WIDTH=512&HEIGHT=512&LAYERS=ORTHO_JPEG_TEST&STYLES=&FORMAT=image/jpeg&DPI=96&TRANSPARENT=TRUE"
sum="`md5sum tmp.jpg | cut -d ' ' -f 1`"
echo $sum
if [ "$sum" = "528542e3f57a1e9b5b80048735ece0a7" ]
then
        echo "Test 1.2.2.2. OK"
else
        echo "Test 1.2.2.2. FAILED"
        let NERRORS+=1
fi
#rm tmp.jpg


# 1.2.2.3. Out Png
let NTESTS+=1
wget  --no-proxy -O tmp.png "$1?SERVICE=WMS&VERSION=1.1.1&REQUEST=GetMap&BBOX=805888,6545920,806400,6546432&CRS=IGNF:LAMB93&WIDTH=512&HEIGHT=512&LAYERS=ORTHO_JPEG_TEST&STYLES=&FORMAT=image/png&DPI=96&TRANSPARENT=TRUE"
sum="`md5sum tmp.png | cut -d ' ' -f 1`"
echo $sum
if [ "$sum" = "af089fe005cd7d9571d2eb3afec46760" ]
then
        echo "Test 1.2.2.3. OK"
else
        echo "Test 1.2.2.3. FAILED"
        let NERRORS+=1
fi
#rm tmp.jpg

# 2. WMTS

# 2.1. GetCapabilitites

# 2.2. GetTile

# 2.2.1. Raw Tile
let NTESTS+=1
wget  --no-proxy -O tmp.tif "$1?SERVICE=WMTS&REQUEST=GetTile&tileCol=6300&tileRow=79932&tileMatrix=0_5&LAYER=SCAN_RAW_TEST&STYLES=&FORMAT=image/tiff&DPI=96&TRANSPARENT=TRUE&TILEMATRIXSET=FR_LAMB93&VERSION=1.0.0"
sum="`md5sum tmp.tif | cut -d ' ' -f 1`"
echo $sum
if [ "$sum" = "bd46ed21e45d5214d59221ca6bf84c29" ]
then
        echo "Test 2.2.1. OK"
else
        echo "Test 2.2.1. FAILED"
        let NERRORS+=1
fi
#rm tmp.tif

# 2.2.2. Jpeg Tile
let NTESTS+=1
wget  --no-proxy -O tmp.jpg "$1?SERVICE=WMTS&REQUEST=GetTile&tileCol=6300&tileRow=79932&tileMatrix=0_5&LAYER=ORTHO_JPEG_TEST&STYLES=&FORMAT=image/jpeg&DPI=96&TRANSPARENT=TRUE&TILEMATRIXSET=FR_LAMB93&VERSION=1.0.0"
sum="`md5sum tmp.jpg | cut -d ' ' -f 1`"
echo $sum
if [ "$sum" = "8f221b367d8302dcdc773698ca780ecd" ]
then
        echo "Test 2.2.2. OK"
else
        echo "Test 2.2.2. FAILED"
        let NERRORS+=1
fi
#rm tmp.jpg

# 2.2.3. Png Tile
let NTESTS+=1
wget  --no-proxy -O tmp.png "$1?SERVICE=WMTS&REQUEST=GetTile&tileCol=6300&tileRow=79932&tileMatrix=0_5&LAYER=PARCELLAIRE_PNG_TEST&STYLES=&FORMAT=image/png&DPI=96&TRANSPARENT=TRUE&TILEMATRIXSET=FR_LAMB93&VERSION=1.0.0"
sum="`md5sum tmp.png | cut -d ' ' -f 1`"
echo $sum
if [ "$sum" = "9f014aaeb661494d03f81859ebcd8f44" ]
then
        echo "Test 2.2.3. OK"
else
        echo "Test 2.2.3. FAILED"
        let NERRORS+=1
fi
#rm tmp.png

echo " ERRORS : "$NERRORS"/"$NTESTS

exit $NERRORS
