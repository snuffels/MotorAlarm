
bushoogte=5;
busdoorsnede=5/2;
busgat=2.5/2;

wanddikte=3;
frontdikte=1.5;
lengte=65; //draad ruimte 4mm
breedte=51; //draad ruimte 4mm
hoogte=26+bushoogte; //hoogte van de module zelf

busx1=13.97;
busy1=8.26;
busx2=2.54;
busy2=58.42;
busx3=44.45;
busy3=58.42;

screenx1=2.36;
screeny1=14.42;
screenx2=31.8;
screeny2=38.9;



translate([0,lengte,frontdikte+hoogte]){rotate([180,0,0]){	doosje();}}
//translate([breedte+10,0,0]){	plaat();}

//doosje();

//nulpunt is de linker onderhoek van de pcb
module doosje()
{
	difference()
	{
		translate([0-wanddikte-1,0-wanddikte-1,0-frontdikte]) {cube([breedte+(wanddikte*2),lengte+(wanddikte*2),hoogte+(frontdikte*2)]);}
		translate([0-1,0-1,0-frontdikte-0.01]) {cube([breedte,lengte,hoogte+frontdikte]);}
//		translate([screenx1,screeny1,hoogte-wanddikte-5]) {cube([screenx2,screeny2,wanddikte*4]);}
		translate([screenx1,lengte-screeny1-screeny2,hoogte-frontdikte-5]) {cube([screenx2,screeny2,wanddikte*4]);}
		//plaat();
		translate([-2,-2,0-frontdikte]) {cube([breedte+2,lengte+2,frontdikte]);}

		translate([breedte,lengte-wanddikte-4,frontdikte+4]) {rotate([0,90,0]) {cylinder(20,4,4,center=true,$fn=12);}}
	}

}

module plaat()
{
	translate([-2,-2,0]) {cube([breedte+2,lengte+2,frontdikte]);}
	translate([busx1,busy1,frontdikte]) {bus();}
	translate([busx2,busy2,frontdikte]) {bus();}
	translate([busx3,busy3,frontdikte]) {bus();}
}

module bus()
{
	difference()
	{
		translate([0,0,(bushoogte/2)-0.1]){cylinder(bushoogte,busdoorsnede,busdoorsnede,center=true);}
		translate([0,0,(bushoogte/2)-0.1]){cylinder(bushoogte+2,busgat,busgat,center=true,$fn=12);}
	}


}