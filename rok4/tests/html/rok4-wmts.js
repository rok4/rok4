var url_template="http://obernai.ign.fr/rok4/bin/rok4?SERVICE=WMTS&REQUEST=GetTile&tileCol=__COL__&tileRow=__ROW__&tileMatrix=__ZOOM__&LAYER=__LAYER__&STYLES=&FORMAT=__FORMAT__&DPI=96&TRANSPARENT=TRUE" ;

var wmts_map={
  col:3715,
  row:40025,
  layer:"__NOLAYER__",
  format:"__NOFORMAT__",
  zoom:9,
  url:"__NOURL__"
};

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

var debuging=false;

function fill_url(i,j) {
    wmts_map.url=url_template.replace(/__COL__/g,wmts_map.col+j-1) ;
    wmts_map.url=wmts_map.url.replace(/__ROW__/g,wmts_map.row+i-1) ;
    wmts_map.url=wmts_map.url.replace(/__LAYER__/g,wmts_map.layer) ;
    wmts_map.url=wmts_map.url.replace(/__FORMAT__/g,wmts_map.format) ;
    wmts_map.url=wmts_map.url.replace(/__ZOOM__/g,matrixIds[wmts_map.zoom].identifier) ;
}

function goLeft() {
	wmts_map.col-=1;
	debug() ;
	redraw() ;
}
function goRight() {
	wmts_map.col+=1;
	debug() ;
	redraw() ;
}
function goUp() {
	wmts_map.row-=1;
	debug() ;
	redraw() ;
}
function goDown() {
	wmts_map.row+=1;
	debug() ;
	redraw() ;
}

function zoomMoins() {
	// recup valeur zoom
	if (wmts_map.zoom<=0) {
		alert ("Impossible de dezoomer plus fort...") ;
		debug() ;
		return ;
	}
	wmts_map.zoom-- ;
	if (wmts_map.col % 2 == 1) {
		wmts_map.col+=1; 
	}
	wmts_map.col= wmts_map.col/2; 
	if (wmts_map.row % 2 == 1) {
		wmts_map.row+=1; 
	}
	wmts_map.row= wmts_map.row/2; 
	debug() ;
	redraw() ;

}
function zoomPlus() {
	// recup valeur zoom
	if (wmts_map.zoom>=10) {
		alert ("Impossible de zoomer plus fort...") ;
		debug() ;
		return ;
	}
	wmts_map.zoom++ ;
	wmts_map.col*=2 ;
	wmts_map.row*=2 ;
	debug() ;
	redraw() ;
}

function debug() {
	if (debuging) alert(
			"col:"+wmts_map.col+"\n"+
			"row:"+wmts_map.row+"\n"+
			"layer:"+wmts_map.layer+"\n"+
			"format:"+wmts_map.format+"\n"+
			"zoom:"+matrixIds[wmts_map.zoom].identifier+"\n",
			"url:"+wmts_map.url+"\n"
			) ;
}

function redraw() {

  // format :
  wmts_map.format= document.getElementById('formatSelect').options[document.getElementById('formatSelect').selectedIndex].value ;
  if (debuging) alert("format : "+wmts_map.format) ;
  // layer :
  wmts_map.layer= document.getElementById('layerSelect').options[document.getElementById('layerSelect').selectedIndex].value ;
  if (debuging) alert("layer : "+wmts_map.layer) ;
  // img
  for (i=0 ; i<=2 ; i++) {
    for (j=0 ; j<=2 ; j++) {
    	var imgElem= document.getElementById("img"+i+""+j) ;
    	if (imgElem==null) alert("img"+i+""+j+" absent ?") ;
    	fill_url(i,j) ;
    	imgElem.src=wmts_map.url ;
    	if (debuging) alert ("imgElem.src="+wmts_map.url) ;
    } //j	
  }// i
  //window.location.reload();
}
