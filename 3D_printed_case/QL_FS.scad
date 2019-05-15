//QL Fileserver Case

translate([0,-35,0]){
    bottom();
    translate([-2,0,20]) middle();
    translate([0,2,31]) top();
    translate([2,2,43]) lid();
}



module bottom(){
    difference(){
       cube([60,70,20]);
        translate([2,2,2]) cube([55,66,18]);
        translate([-1,31,4]) BluePill_usb();
        translate([-1,20,14]) jack();
        translate([-1,50,14]) jack();
        translate([-1,32,10]) reset_button();
        translate([59,18,7.5]) vent_slot();
    }
    difference(){
        translate([2,2,18]) cube([56,66,4]);
        translate([4,4,18]) cube([52,62,5]);
    }
    translate([2,22.5,2]) BluePill_mount();
    
}

module middle(){
    difference(){
        cube([72,70,11]);
            translate ([4,2,2]) cube([66,66,9]);
            translate([4,2,0]) cube([56,66,18]);
            translate([-0,18,0]) vent_slot_back();
    }
   translate([9,2,9])  cube([44,2,4]);
   translate([9,66,9])  cube([44,2,4]);
}

module top(){
    difference(){
        translate([0,-2,0]) cube([70,70,2]);
        translate([2,0,0]) cube([54,66,2]);
        translate([59,12,-8]) led();
    }
    difference(){
        translate([0,-2,2]) cube([58,70,12]);
        translate([2,0,0]) cube([54,66,15]);
        translate([54,18,7]) sdslot();
    }
    translate([46,18,3]) cube([10,30,2]);
    translate([2,0,0]) lid_mounting();
    translate([2,61,0]) lid_mounting();
    translate([51,0,0]) lid_mounting();
    translate([51,61,0]) lid_mounting();
}

module lid(){
    difference(){
       cube([53.8,65.8,2]);
        translate([2,0,0])  lid_hole();
        translate([2,61,0]) lid_hole();
        translate([51,0,0]) lid_hole();
        translate([51,61,0]) lid_hole();
        for(a=[11:15:50]) translate([a,0,1.5]) #cube([0.5,66,0.5]);
       
    }
    translate([7,26,3]) logo();
}

module logo(){
    rotate([0,0,90])
    scale([0.1, 0.1, 0.01])
    surface(file = "sinclair-logo.png", center = true, invert = true);
}

module BluePill_mount(){
    cube([55,3,2]);
    cube([55,1,4]);
    translate([0,24,0]) cube([55,1,4]);
    translate([0,22,0]) cube([55,3,2]);
}

module BluePill_usb(){
    #cube([5,8,3]);
}

module lid_hole(){
    #translate([0.5,2.5,0]) {
        cylinder(h=2,d=1.5,$fn=10);
        translate([0,0,1]) cylinder(h=1,d=2.5,$fn=20);
    }
}

module lid_mounting(){
    difference(){
        cube([5,5,12]);
        #translate([2.5,2.5,7]) cylinder(h=5,d=1.5,$fn=10);
    }
}
module sdslot(){
    #cube([5,30,3.2]);
}

module led(){
    #cube([5.2,2.2,10]);
}

module jack(){
    #rotate([0,90,0]) cylinder(d=6.3,h=6,$fn=30);
}

module reset_button(){
    #cube([4,6.2,6.2]);
}
module vent_slot(){
    for(a=[0:4:35]) translate([0,a,0]) #cube([1,2,12]);
}

module vent_slot_back(){
    for(a=[0:4:35]){
        translate([0,a,0]) #cube([1,2,11]);
    }
}