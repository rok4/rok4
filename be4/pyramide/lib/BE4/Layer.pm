# TODO

=head1 SAMPLE

* Sample Pyramid file (.pyr) :

  eg SEE ASLO

* Sample TMS file (.tms) :

  eg SEE ASLO

* Sample LAYER file (.lay) :

  [SCAN_RAW_TEST.lay]
  
  <layer>
	<title>Scan IGN en RAW</title>
	<abstract></abstract>
	<keywordList>
		<keyword></keyword>
	</keywordList>
	<style>normal</style>
	<minRes>0.5</minRes>
	<maxRes>1</maxRes>
	<EX_GeographicBoundingBox>
                <westBoundLongitude>0</westBoundLongitude>
                <eastBoundLongitude>10</eastBoundLongitude>
                <southBoundLatitude>40</southBoundLatitude>
                <northBoundLatitude>50</northBoundLatitude>
        </EX_GeographicBoundingBox>
	<WMSCRSList>
		<WMSCRS>IGNF:LAMB93</WMSCRS>
	</WMSCRSList>
	<boundingBox CRS="IGNF:LAMB93" minx="805888" miny="6545408" maxx="806912" maxy="6546432"/>
	<opaque>true</opaque>
	<authority>IGNF</authority>
	<resampling>lanczos_4</resampling>
	<pyramidList>
		<pyramid>../config/pyramids/SCAN_RAW_TEST.pyr</pyramid>
	</pyramidList>
  </layer>

=head1 SEE ALSO

 eg package module following :
 
 BE4::Pyramid
 BE4::TileMatrixSet

