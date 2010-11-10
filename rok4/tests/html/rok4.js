var map;

function init() {
    // limites definies par le TMS de la couche
    var bounds= new OpenLayers.Bounds() ;
    bounds.extend(new OpenLayers.LonLat(1331200, 7127296));
    bounds.extend(new OpenLayers.LonLat(0, 5999872));

    map = new OpenLayers.Map({
        div: "map",
        projection: "EPSG:2154",
        units: "m",
        maxExtent: bounds,
        maxResolution: 512,
        minResolution: 0.5,
        center: new OpenLayers.LonLat(657750, 6860651),
        zoom:9,
        numZoomLevel: 11
    });    


/*
 *  Tentative d'ajout du layer WMTS ... reporte a + tard !
 *
    // If tile matrix identifiers differ from zoom levels (0, 1, 2, ...)
    //
    // then they must be explicitly provided.
    var matrixIds = new Array(11);
    matrixIds[10]= { identifier: "0_5" } ;
    matrixIds[9]=  { identifier: "1" } ;
    matrixIds[8]=  { identifier: "2" } ;
    matrixIds[7]=  { identifier: "4" } ;
    matrixIds[6]=  { identifier: "8" } ;
    matrixIds[4]=  { identifier: "32" } ;
    matrixIds[5]=  { identifier: "16" } ;
    matrixIds[3]=  { identifier: "64" } ;
    matrixIds[2]=  { identifier: "128" } ;
    matrixIds[1]=  { identifier: "256" } ;
    matrixIds[0]=  { identifier: "512" } ;

// http://localhost/gilles/cgi-bin/wmsserver/bin/rok4?SERVICE=WMTS&REQUEST=GetTile&tileCol=3716&tileRow=40026&tileMatrix=1&LAYERS=ORTHO&STYLES=&FORMAT=image/tif&DPI=96&TRANSPARENT=TRUE

alert ("create WMTS Layer") ;

    var wmts = new OpenLayers.Layer.WMTS({
        isBaseLayer: true,           // {Boolean} The layer will be considered a base layer.
        requestEncoding: 'KVP',      // {String} Request encoding.  Can be “REST” or “KVP”.  Default is “KVP”.
        url: "http://localhost/gilles/cgi-bin/wmsserver/bin/rok4", // {String} The base URL for the WMTS service. 
        layer: "ORTHO",              // {String} The layer identifier advertised by the WMTS service.
        matrixSet: "FR_LAMB93_test", // {String} One of the advertised matrix set identifiers.
        style: "_null",              // {String} One of the advertised layer styles. 
        format: "image/tif",         // {String} The image MIME type.
        tileOrigin: new OpenLayers.LonLat(0, 16777216), // {OpenLayers.LonLat} The top-left corner of the tile matrix in map units.
        maxExtent: bounds,
//        tileFullExtent:,              // {OpenLayers.Bounds} The full extent of the tile set. If not supplied, the layer’s maxExtent property will be used.
        matrixIds: matrixIds,        // {Array} A list of tile matrix identifiers. 
        name: "Ortho IGN en jpeg",
        opacity: 1
    });                
*/

/*
    var wmsOJpeg= new OpenLayers.Layer.WMS(
      "(WMS) ORTHO JPEG",
      "http://obernai.ign.fr/rok4/bin/rok4?",
      {layers:"ORTHO"}
    ) ; 
*/
    var wmsOParis= new OpenLayers.Layer.WMS(
      "(WMS) ORTHO RAW",
      "http://obernai.ign.fr/rok4/bin/rok4?",
      {layers:"ORTHO_RAW", version:"1.3"}
    ) ; 
/*
    var wmsOTiff= new OpenLayers.Layer.WMS(
      "(WMS) ORTHO TIFF",
      "http://obernai.ign.fr/rok4/bin/rok4?",
      {layers:"ORTHO_TIFF"}
    ) ; 
*/

    map.addLayers([wmsOParis/*,wmsOTiff*/]);
    map.addControl(new OpenLayers.Control.LayerSwitcher());

    //alert("bounds : "+map.calculateBounds().toString()+"\ncenter: "+map.getCenter()) ;

}
