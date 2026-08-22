########################
#                      #
#  Tiberiumsilo-Setup  #
#                      #
########################

### Content
1. Beschreibung - Description
2. Installation

### 1. Description

  Das ist meine Variante des Tiberiumsilo-Setups für Renegade-Maps. Korrekt eingebaut sieht man deutliche Beschädigungen und Rauch bei wenig Leben.
  Die Silos sind tauglich für flying maps (also man kann nicht auf deren "Dach" landen) und besitzen zwei kleine Rampen für flying maps um in das kleine Abteil mit den Rohren zu gelangen.

  This my variant of an tiberiumsilo setup for Renegade maps. Correctly installed you can see clear damages and smoke when it has low health.
  The silos are ready for flying maps (you can't land on their roofs) and they have two little ramps to get inside the little area where pipes are.


### 2. Installation

  -Tiberiumsilos in die Gmax-Szene laden und platzieren
  -Terrain im Leveleditor laden
  -hinzufügen der Helipad-Aggregate:
	+ zwei Temp-Kopien des Preseteintrags "gdiwep_ag_1" machen
	+ die eine "mgsil_ag_1c3" nennen und die andere "mnsil_ag_1c3"
	+ die Einträge editieren so dass diese auf die entsprechenden .w3d's mit demselben Namen verweisen (die w3ds sind in diesem Archiv fertig entahlten)
  -als Vorlage für den Buildingcontroller (die blauen Häuschen) nehmt den der Raffinerie
  -macht also jeweils von GDI- und Nod-Werkstatt eine Temp-Kopie und benennt sie wie ihr wollt
  -setzt das MESHPREFIX für den GDI-Controller auf MGSIL und für den Nod-Controller auf MNSIL
  -setzt Leben/Panzerung auf 250/250
  -setzt die Deathpoints auf 100
  -setzt FundsperSec auf 1
  -setzt FundsGathered auf 0
  -macht den Eintrag "Harvester" leer (auf none setzen)

  -nehmt unbedingt die Textur-Datei "ref_win4gdi.dds" in euer .mix MapArchiv auf!



  -load the Ttiberiumsilos into you Gmax scene
  -load you terrain into Leveleditor
  -adding the helipad-aggregates
	+ make two temp-copies of "gdiwep_ag_1"
	+ name one "mgsil_ag_1c3" and the other "mnsil_ag_1c3"
	+ edit these entries that they point at the .w3d's with corresponding names (they .w3ds you can find in this archive here ready to use)
  -use the repair bay buildingcontrollers (small blue houses) as template for the refinery controllers
  -so make a temp copy of GDI and Nod controller und give them a name (any name you want)
  -make sure that you set the MESHPREFIX for the GDI controller to MGSIL and for the Nod controller to MNSIL
  -set Health/Armor to 250/250
  -set Deathpoints to 100
  -set FundsperSec to 1
  -set FundsGathered to 0
  -clear the "Harvester" lentry (set it to none)

  -add the texture file "ref_win4gdi.dds" to your .mix maparchive!