##############################
#                            #
#  Construction-Yards-Setup  #
#                            #
##############################

### Content
1. Beschreibung - Description
2. Installation


### 1. Description

  Dies ist mein Bauhof-Setup. Obwohl viele Ähnlichkeiten zu Setups auf anderen Maps von anderen Autoren gibt, hab ich das ganze Setup hier
  allein durchgeführt - ohne Daten andere zu benutzen! Ich hab nur ihre Idee z.t. mit übernommen.

	Feature-Liste:
	-komplett Flying-tauglich
	-alle Gebäude-Aggregate modifizert bzw. verbessert
	-Zerstörungs-Animation (innen wie außen) mit Emittern und Rissen

  Ich hab außerdem mal zwei testmaps mit reingepackt wo man sich das fertige Setup ansehen kann.

  WICHTIG!!!!	Wenn du die Aggregate für eigene Zwecke modifizieren willst dann benenne deine Versionen bitte um z.B. statt dem c in mgcon_ag_2c
		halt ein d (also mgcon_ag_2d) oder anderes. Das gilt für alle Aggregate! Ich will keine Überschneidungen mit meinen Gebäude-Aggregaten!



  This is my Construction-Yard setup. Although there are lots of similartities to other setups from other authors, I did the whole setup alone -
  without using data from others! I only used some of their ideas. 
	Feature-List:
	-completly suitable for flying-maps
	-modified/improved every building-aggregate
	-destruction-animation (inside and outside) with emitters und cracks

  I also packed some testmaps in here where you can take a look at may setup.

  IMPORTANT!!!!	If you want to modify the aggregates for your purposes, then please rename your modded versions e.g. instead of a c in mgcon_ag_2c
		use a d (like this mgcon_ag_2d). This applies for all the other aggregates too! I don't want any intersections with my aggregtates!

### 2. Installation

  -wenn ich von "Datei selben Namens verweisen" spreche, dann meine ich die entsprechende .w3d Datei
  -Bauhöfe in die Gmax-Szene laden und platzieren
  -Terrain im Leveleditor laden
  -hinzufügen der Bauhof-Aggregate:

	+geht in der Presetsliste zu Terrain => Building Interior Tiles => mp - GDI Construction Yard
		-Temp-Duplikat des Eintrags "mgcon_doors" erstellen und "mgcon_doors_c" nennen
		-dieser soll auf die entsprechende .w3d Datei selben Namens verweisen
		-weitere Einträge der folgenden Namen erstellen und verknüpfen mit den entsprechenden Dateien
			"mgcon_int_c"
			"mgcon_vis_c"
			"mgcon_visx_c"

	+geht in der Presetsliste zu Terrain => Building Interior Tiles => mp - Nod Construction Yard
		-Temp-Duplikat des Eintrags "mncon_doors" erstellen und "mncon_doors_c" nennen
		-dieser soll auf die entsprechende .w3d Datei selben Namens verweisen
		-Wiederholt das mit den Einträgen analog
			"mncon_int" => "mncon_int_c"
			"mncon_vis" => "mncon_vis_c"
			"mncon_visx" => "mncon_visx_c"

	+geht in der Presetsliste zu Tile => Building Aggregates - Multi Player => GDI Construction Yard
		-Temp-Duplikat des Eintrags "mgcon_ag_2" erstellen und "mgcon_ag_2c" nennen
		-dieser soll auf die entsprechende .w3d Datei selben Namens verweisen
		-Wiederholt das mit den Einträgen analog
			"mgcon_ag_3" => "mgcon_ag_3c"
		-für das Aggregat "mgcon_ag_1c" wird das "mgwep_ag_1" der Waffenfrabrik als Vorlage genommen!!!
		 (ist hier zu finden Tile => Building Aggregates - Multi Player => GDI Weaponsfactory)

	+geht in der Presetsliste zu Tile => Building Aggregates - Multi Player => Nod Construction Yard
		-verfahrt hier genauso wie mit dem GDI Bauhof nur hinzu kommt hier noch:
		-Temp-Duplikat des Eintrags "mncon_lt" erstellen und "mncon_lt_c" nennen
		-dieser soll auf die entsprechende .w3d Datei selben Namens verweisen

  -nun muss noch ein Buildingcontroller für den Bauhof gemacht werden
	+geht zu Buildings => Powerplant
	+Temp-Duplikat der Eintrags beiden Einträge erstellen und "mp_GDI-ConYard" / "mp_Nod-ConYard" nennen
	+ändert nun folgenden Eigenschaften der Einträge durch "Mod"
		-Meshprefix "MGCON" für GDI und "MNCON" für Nod
		-Building-Type "Construction Yard"
	+jetzt müsst ihr noch die Damage/DestroyReports ändern damit auch die Meldungen kommen
	+damit die Bauhöfe auch die Gebäude reparieren müssten noch ein paar Scripts angehangen werden
	+erfragt bitte genauerer Details im Forum zu den letzten beiden Schritten falls unklar


  -when I speak of "entry with the file of the same name" I mean the .w3d file with the same name
  -load and place the ConYards into your Gmax-Scene
  -load terrain in Leveleditor
  -adding ConYard building aggregates:

	+go to Terrain => Building Interior Tiles => mp - GDI Construction Yard in the presetslist
		-create Temp-Duplicate of entry "mgcon_doors" and name it "mgcon_doors_c"
		-connect this entry with the file of the same name
		-create further entries with the following names and connect them with their files
			"mgcon_int_c"
			"mgcon_vis_c"
			"mgcon_visx_c"

	+go to Terrain => Building Interior Tiles => mp - Nod Construction Yard in the presetslist
		-create Temp-Duplicate of entry "mncon_doors" and name it "mncon_doors_c"
		-connect this entry with the file of the same name
		-repeat that the following entries in the same way
			"mncon_int" => "mncon_int_c"
			"mncon_vis" => "mncon_vis_c"
			"mncon_visx" => "mncon_visx_c"

	+go to zu Tile => Building Aggregates - Multi Player => GDI Construction Yard in the presetslist
		-create Temp-Duplicate of entry "mgcon_ag_2" and name it "mgcon_ag_2c"
		-connect this entry with the file of the same name
		-repeat that the following entries in the same way
			"mgcon_ag_3" => "mgcon_ag_3c"
		-"mgcon_ag_1c" is different!!! use "mgwep_ag_1" of the GDI Weapons factory as template!!!
		 (you can find it here Tile => Building Aggregates - Multi Player => GDI Weaponsfactory)

	+go to Tile => Building Aggregates - Multi Player => Nod Construction Yard in the presetslist
		-follow the instructions of GDI Conyard, its that same here but one more thing is to do here:
		-create Temp-Duplicate of entry "mncon_lt" and name it "mncon_lt_c"
		-connect this entry with the file of the same name

  -Buildingcontroller setup
	+go to Buildings => Powerplant
	+create Temp-Duplicate of both entries there and name them "mp_GDI-ConYard" / "mp_Nod-ConYard"
	+ächange the following settings via "Mod"
		-Meshprefix "MGCON" for GDI and "MNCON" füor Nod
		-Building-Type "Construction Yard"
	+change Damage/Destroy ReportIDs for the EVAG/EVAN Messages
	+you have to add some scripts (dunno which ones) without them the conyards don't repair the other buildings
	+ask in the forums for further details (especially the last two steps)
