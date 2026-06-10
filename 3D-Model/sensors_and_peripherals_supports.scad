$fn=100;
$fa=0.1;
//comprehensive of the diameters of the holes
board_hole_horiz_dist = 56.45;
board_hole_diameter = 3;
height_to_board = 8.3;
board_width = 58.5;
board_thickness=1.5;
base_length=30;
tolerance=0.01;

base_wall_external=2;

hole_perimeter_to_board_edge=1.2;

notch_width=5;
notch_length=3;
notch_thickness=1;
notch_height=board_thickness*1.0;




RFID_hole_to_board_distance=25;
RFID_width=40;
RFID_length=60-12.8;



vl53l0x_height=37; //from board top




// apply tolerances
fixed_board_hole_r=board_hole_diameter/2-tolerance;
fixed_height_to_board=height_to_board-tolerance;

hole_cone_base_r=fixed_board_hole_r*0.9;
hole_cone_top_r=fixed_board_hole_r*0.7;





//base cube
module base_cube(width=board_width,length=base_length,height=fixed_height_to_board){
        difference(){
            cube([width,
                length,
                height], 
                center=true);
 
            translate([0,0,-base_wall_external])
            cube([width - 2*base_wall_external,
                length - 2*base_wall_external,
                height], 
                center=true);
        }
}

//cones that will slide in the holes
module hole_cone(offset_x=0,offset_y=0,offset_z=0){
    translate([offset_x,offset_y,offset_z])
        cylinder(r1=hole_cone_base_r,
            r2=hole_cone_top_r,
            h=board_thickness*1.0,
            center=true);
}


module notch(offset_x=0,offset_y=0,offset_z=0, rotation_z=0){

    p0=[0,0];
    p1=[notch_thickness,0];
    p2=[notch_thickness,-notch_height];
    p3=[notch_thickness+notch_length,-(notch_height+notch_thickness)*0.75];
    p4=[notch_thickness+notch_length,-(notch_height+notch_thickness)];
    p5=[0,-(notch_height+notch_thickness)];
    
    translate([offset_x,offset_y,offset_z])
    rotate([-90,0,-90+rotation_z])
    linear_extrude(height=notch_width, center=true)
    polygon([p0,p1,p2,p3,p4,p5]);
}

//base that inserts on the board
module board_base(offset_x=0,offset_y=0,offset_z=0){
    
    cone_x_offset=(board_hole_horiz_dist -board_hole_diameter)/2;
    
    cone_y_offset=(base_length/2)-hole_perimeter_to_board_edge-hole_cone_base_r-notch_thickness*1.1;
    
    cone_z_offset=(fixed_height_to_board/2)+board_thickness/4-0.001;
    
    translate([offset_x,offset_y,offset_z]){
        base_cube();
        
        hole_cone(offset_x=cone_x_offset,
            offset_y=cone_y_offset,
            offset_z=cone_z_offset);
        hole_cone(offset_x=-cone_x_offset,
            offset_y=cone_y_offset,
            offset_z=cone_z_offset);
        
        notch(0,base_length/2,fixed_height_to_board/2-0.001);
        
    }
}





module RFID(){
	//calculate size with notches
	width=RFID_width+2*(notch_thickness+tolerance);
	length=RFID_length+notch_thickness+tolerance;

	arm_width=5; // note the usb plug allows for max 5mm
	translate([arm_width/2-board_width/2,RFID_hole_to_board_distance/2-0.001+base_length/2,0])
	base_cube(arm_width,RFID_hole_to_board_distance);
	
	translate([
	width/2-board_width/2+arm_width-width,
	length/2+base_length/2+RFID_hole_to_board_distance-0.001,
	0]){
		base_cube(width,length);
		
		translate([0,length/2,fixed_height_to_board/2-0.001])
		notch();
		
		translate([width/2,0,fixed_height_to_board/2-0.001])
		notch(rotation_z=-90);
		
		translate([-width/2,0,fixed_height_to_board/2-0.001])
		notch(rotation_z=90);
	}
}



module vl53l0x(){
	
	height=vl53l0x_height+height_to_board+board_thickness;
	width=2;
	vl53l0x_r=6.87;
	
	translate([0,width/2+base_length/2-0.001,(height-height_to_board)/2]){
		cube([5,width,height], center=true);
		
		translate([-10,0,height/2+vl53l0x_r])
		rotate([90,0,0]){
			
			linear_extrude(height=width, center=true)
			hull(){
				translate([20,0])
				circle(r=vl53l0x_r);
				
				circle(r=vl53l0x_r);	
			}
			
			translate([0,0,7/2])
			linear_extrude(height=5, center=true){
				circle(r=1.9);
				
				translate([20,0])
				circle(r=1.92);
			}
		}
	}
}





module complete(){
	board_base();
	RFID();
	
	vl53l0x();
}




complete();

