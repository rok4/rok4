
NERRORS=0

# 1. WMS

# 1.1. GetCapabilitites

# 1.2. GetMap

# 1.2.1 In Raw

# 1.2.1.1 Out Raw

# 1.2.1.1.1. Res < resmin

# 1.2.1.1.2. resmin < Res < resmax

wget  --no-proxy -O tmp.tif "http://localhost/$1?SERVICE=WMS&VERSION=1.1.1&REQUEST=GetMap&BBOX=805888,6545920,806400,6546432&CRS=IGNF:LAMB93&WIDTH=512&HEIGHT=512&LAYERS=ORTHO_JPEG_TEST&STYLES=&FORMAT=image/tiff&DPI=96&TRANSPARENT=TRUE"

sum="`md5sum tmp.tif | cut -d ' ' -f 1`"
echo $sum
if [ "$sum" = "b70ad3e72e0b66710ca2b93d25f7bfd9" ]
then
        echo "Echec Test 1.2. "
        $NERRORS=$NERRORS+1
fi

# 1.2.1.1.3. Res > resmax




# 2. WMTS

# 2.1. GetCapabilitites

# 2.2. GetTile

# 2.2.1. In : Raw ; Out : Raw

# 2.2.2. In : Jpeg ; Out : Jpeg

# 2.2.3. In Png ; Out : Png


