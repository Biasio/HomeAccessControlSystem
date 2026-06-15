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
notch_thickness=3;
notch_height=board_thickness*1.1;




RFID_hole_to_board_distance=25;
RFID_width=40;
RFID_length=60-12.8;



vl53l0x_height=60; //from board top




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


module notch(offset_x=0,offset_y=0,offset_z=0, rotation_z=0, height=notch_height, angle=0.7){

    p0=[0,0];
    p1=[notch_thickness,0];
    p2=[notch_thickness,-height];
    p3=[notch_thickness+notch_length,-(height+notch_thickness)*angle];
    p4=[notch_thickness+notch_length,-(height+notch_thickness)];
    p5=[0,-(height+notch_thickness)];
    
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





module motor_driver(){
	//calculate size with notches
	motor_board_to_board_distance=23.5;
	motor_width=35.2;
	motor_length=31.5;
	motor_height=3.4;
	width=motor_width+2*(notch_thickness+tolerance);
	length=motor_length+notch_thickness+tolerance;

	arm_width=7; 
	translate([-(arm_width/2-board_width/2),motor_board_to_board_distance/2-0.001+base_length/2,0])
	base_cube(arm_width,motor_board_to_board_distance);
	
	translate([
	(-width/2+board_width/2+width/2-arm_width/2),
	length/2+base_length/2+motor_board_to_board_distance-0.001,
	0]){
		base_cube(width,length);
		
		translate([0,length/2,fixed_height_to_board/2-0.001])
		notch(height=motor_height);
		
		translate([width/2,0,fixed_height_to_board/2-0.001])
		notch(rotation_z=-90,height=motor_height);
		
		translate([-width/2,0,fixed_height_to_board/2-0.001])
		notch(rotation_z=90,height=motor_height);
	}
}



module vl53l0x(){

	height=vl53l0x_height+height_to_board+board_thickness;
	width=5;
	vl53l0x_holes_r=(1.42/2);
	vl53l0x_outer_r=6.8/2;
	vl53l0x_width=20-vl53l0x_outer_r*2;
	vl53l0x_pin_distance=12.4+vl53l0x_holes_r*2-0.1;

	translate([0,width/2+base_length/2-0.001,(height-height_to_board)/2]){
		cube([5,width,height], center=true);

		translate([-vl53l0x_width/2,0,height/2+vl53l0x_outer_r])
		rotate([90,0,0]){

			linear_extrude(height=width, center=true)
			hull(){
				translate([vl53l0x_width,0])
				circle(r=vl53l0x_outer_r);

				circle(r=vl53l0x_outer_r);	
			}

			translate([0,0,5.7/2+width/2])
			linear_extrude(height=5.7, center =true){
				translate([vl53l0x_pin_distance,0,0])
				circle(r=vl53l0x_holes_r);

				circle(r=vl53l0x_holes_r);
			}
		}
	}
}





module complete_top(){
	board_base();
	RFID();
	motor_driver();
	vl53l0x();
}




module breadboard(){
	//calculate size with notches
	breadboard_board_to_board_distance=3.2;
	breadboard_width=85.82;
	breadboard_length=56.5;
	breadboard_height=9.9;
	
	width=breadboard_width+2*(notch_thickness+tolerance);
	length=breadboard_length+notch_thickness+tolerance;

	arm_width=15; 
	translate([0,breadboard_board_to_board_distance/2-0.001+base_length/2,0])
	base_cube(arm_width,breadboard_board_to_board_distance);
	
	translate([
	0,
	length/2+base_length/2+breadboard_board_to_board_distance-0.001,
	0]){
		base_cube(width,length);
		
		translate([0,length/2,fixed_height_to_board/2-0.001])
		notch(height=breadboard_height,angle=0.9);
		
		translate([width/2,0,fixed_height_to_board/2-0.001])
		notch(rotation_z=-90,height=breadboard_height,angle=0.9);
		
		translate([-width/2,0,fixed_height_to_board/2-0.001])
		notch(rotation_z=90,height=breadboard_height,angle=0.9);
	}
}



module complete_bottom(){
	
	translate([-100,-50,0]){
		rotate([0,0,0])
		board_base();
		
		rotate([0,0,0])
		breadboard();
	}
}


//complete_bottom();
complete_top();

