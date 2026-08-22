###################
#                 #
#  Helipad-Setup  #
#                 #
###################

### Content
1. Beschreibung - Description
2. Installation


### 1. Description

  Das ist meine Variante des Helipad-Setups für Renegade-Maps. Korrekt eingebaut sieht man deutliche Beschädigungen und Rauch bei wenig Leben.

  This my variant of an helipad setup for Renegade maps. Correctly installed you can see clear damages and smoke when the helipads has low health.


### 2. Installation

  -Helipads in die Gmax-Szene laden und platzieren
  -Terrain im Leveleditor laden
  -hinzufügen der Helipad-Aggregate:
	+ zwei Temp-Kopien des Preseteintrags "gdiwep_ag_1" machen
	+ die eine "mghel_ag_1c3" nennen und die andere "mnhel_ag_1c3"
	+ die Einträge editieren so dass diese auf die entsprechenden .w3d's mit demselben Namen verweisen (die w3ds sind in diesem Archiv fertig entahlten)
  -als Vorlage für den Buildingcontroller (die blauen Häuschen) nehmt den der Werkstatt
  -macht also jeweils von GDI- und Nod-Werkstatt eine Temp-Kopie und benennt sie wie ihr wollt
  -setzt das MESHPREFIX für den GDI-Controller auf MGHEL und für den Nod-Controller auf MNHEL
  -tragt als Deathpoints 100 (statt 1000) ein - isn guter Wert


  -load the helipads into you Gmax scene
  -load you terrain into Leveleditor
  -adding the helipad-aggregates
	+ make two temp-copies of "gdiwep_ag_1"
	+ name one "mghel_ag_1c3" and the other "mnhel_ag_1c3"
	+ edit these entries that they point at the .w3d's with corresponding names (they .w3ds you can find in this archive here ready to use)
  -use the repair bay buildingcontrollers (small blue houses) as template for the helipad controllers
  -so make a temp copy of GDI and Nod controller und give them a name (any name you want)
  -make sure that you set the MESHPREFIX for the GDI controller to MGHEL and for the Nod controller to MNHEL
  -set the deathpoints to 100 (instead of 1000) i think it's a good value