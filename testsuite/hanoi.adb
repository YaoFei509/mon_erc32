-------------------------------------------------------------------------
--
-- Creator: Colin Runciman
--
-- Creation Date: 10 March 1982
--
--
-- Abstract:
--    The Towers of Hanoi Problem
--
-- Description:
--    This program demonstrates the solution to the Towers of Hanoi
--    problem.
--
-- Revision History:
--
--    Revision 1.2  1998/12/03 18:03:45  lcj
--    Added comments, formatting
--    
--    Revision 1.1  1982/03/10 16:25:31  cr
--    Initial revision
--
-- DDC-I Demo Program
-- DDC-I PROPRIETARY INFORMATION:
-- Copyright (C) 1997 DDC International A/S.
-- All rights reserved. This material contains unpublished
-- trade secret information from DDC International A/S.
-- TO BE TREATED IN CONFIDENCE
--
-------------------------------------------------------------------------

-- for XGC Ada I/O  -- Yao Fei
with XGC.Text_Io;  use XGC.Text_Io;

procedure HANOI is

   -------------------------------------------
   -- Definitions for screen I/O
   -------------------------------------------

   SCREEN_DEPTH	: constant INTEGER	:= 23;
   SCREEN_WIDTH	: constant INTEGER	:= 80;

   subtype DEPTH is INTEGER range 1..SCREEN_DEPTH;
   subtype WIDTH is INTEGER range 1..SCREEN_WIDTH;

   -------------------------------------------
   -- Data structures to represent the problem
   -------------------------------------------

   POLE_HEIGHT : constant INTEGER   := 7;
   MAX_RING_NO : constant INTEGER   := POLE_HEIGHT - 1;
   TERM_CH     :          CHARACTER :='V';
   BASE_DEPTH  : constant INTEGER   := SCREEN_DEPTH - 1;

   subtype POLE_NO is INTEGER range 1..3;
   subtype HEIGHT  is INTEGER range 0..POLE_HEIGHT;
   subtype RING_NO is INTEGER range 0..MAX_RING_NO;

   type RING_ARRAY is array ( INTEGER range <> ) of RING_NO;

   type POLE_WITH_RINGS is
      record
	 RINGS       : RING_ARRAY( 1..POLE_HEIGHT );
	 RING_HEIGHT : RING_NO;
      end record;

   POLES : array( POLE_NO ) of POLE_WITH_RINGS := (
						   (RINGS => ( 6, 5, 4, 3, 2, 1, 0 ), RING_HEIGHT => MAX_RING_NO ),
						   (RINGS => ( 0, 0, 0, 0, 0, 0, 0 ), RING_HEIGHT => 0 ),
						   (RINGS => ( 0, 0, 0, 0, 0, 0, 0 ), RING_HEIGHT => 0 )
						  );

   ---------------------------------------------------------------------------
   -- name: FIX_CURSOR
   --
   -- purpose:  Display the cursor properly based on the terminal type.
   --
   -- notes: none
   ---------------------------------------------------------------------------
   procedure FIX_CURSOR( D : DEPTH; A : WIDTH ) is

      ---------------------------------------------------------------------------
      -- name: UNSIGNED 
      --
      -- purpose:
      --
      -- notes:
      ---------------------------------------------------------------------------
      function UNSIGNED ( I : INTEGER ) return STRING is
      begin
	 return INTEGER'Image(I) (2..INTEGER'Image(I)'Length);
      end UNSIGNED;

   begin -- FIX_CURSOR

      NEW_LINE(1);  -- in order to empty buffer
      Put( Ascii.ESC & "[" );
      Put( UNSIGNED(D));
      Put( ';' );
      Put( UNSIGNED(A));
      Put( 'f' );
   end FIX_CURSOR;

   ---------------------------------------------------------------------------
   -- name: CLEAR_SCREEN
   --
   -- purpose: Clear the screen for output.
   --
   -- notes: none.
   ---------------------------------------------------------------------------
   procedure CLEAR_SCREEN is
   begin
      Put( Ascii.ESC & "[2J" );
   end CLEAR_SCREEN;


   ---------------------------------------------------------------------------
   -- name: DRAW_RING 
   --
   -- purpose:  Draws a ring on a pole at the appropriate height.
   --
   -- notes: none
   ---------------------------------------------------------------------------
   procedure DRAW_RING( P : POLE_NO; H : HEIGHT; R : RING_NO ) is

      type LAYER is ( TOP, BOTTOM );
      DOWN         : constant INTEGER := BASE_DEPTH - H * 2;
      ACROSS       : constant INTEGER := ( P-1 ) * 26 + 2;
      RING_PICTURE : constant array( RING_NO, LAYER ) of STRING( 1..14 ) := (
									     ("      ||      ",
									      "      ||      "),
									     ("      --      ",
									      "     |01|     "),
									     ("     ----     ",
									      "    | 02 |    "),
									     ("    ------    ",
									      "   |  03  |   "),
									     ("   --------   ",
									      "  |   04   |  "),
									     ("  ----------  ",
									      " |    05    | "),
									     (" ------------ ",
									      "|     06     |")
									    );
   begin
      -- minimise cursor movement/character traffic
      -- drawing non-zero ring never over-writes a larger ring, so can
      -- send minimal amount
      if R = 0 then
	 FIX_CURSOR( DOWN,     ACROSS ); 
	 Put( RING_PICTURE( R, TOP ) );
	 FIX_CURSOR( DOWN + 1, ACROSS ); 
	 Put( RING_PICTURE( R, BOTTOM ) );

      else
	 -- top of picture is two chars smaller than bottom
	 FIX_CURSOR( DOWN,     ACROSS + 7 - R );	                   -- include leading blanks
	 Put( RING_PICTURE( R, TOP ) ( 8 - R .. 7 + R ) );    -- slice central part
	 FIX_CURSOR( DOWN + 1, ACROSS + 6 - R );
	 Put( RING_PICTURE( R, BOTTOM ) ( 7 - R .. 8 + R ) ); -- slice central part

      end if;
   end DRAW_RING;

   ---------------------------------------------------------------------------
   -- name: DRAW_START
   --
   -- purpose: Draw the Towers of Hanoi at that starting position for the problem.
   --
   -- notes: none
   ---------------------------------------------------------------------------
   procedure DRAW_START is
   begin
      CLEAR_SCREEN;
      FIX_CURSOR( 2, 1 ); 
      Put( "-- Towers of Hanoi --" );
      FIX_CURSOR( BASE_DEPTH, 1 );
      
      for I in 1..66 loop 
	 Put( '=' ); 
      end loop;
      
      for P in POLE_NO loop
	 for R in RING_NO loop
	    DRAW_RING( P, POLE_HEIGHT - R, RING_NO'( 0 ) );
	 end loop;
      end loop;
      
      for R in +1..MAX_RING_NO loop
	 DRAW_RING( 1, POLE_HEIGHT - R, R );
      end loop;
   end DRAW_START;


   ---------------------------------------------------------------------------
   -- name: SOLVE 
   --
   -- purpose: expresses a suitable variant of the usual recursive solution.
   --
   -- notes: none
   ---------------------------------------------------------------------------
   procedure SOLVE( R : RING_NO; START_POLE, FINISH_POLE : POLE_NO ) is

      ---------------------------------------------------------------------------
      -- name: MOVE_RING
      --
      -- purpose: Moves a ring from the pole number specifed by the FROM
      --           parameter to the pole number specified by the TO parameter.
      --
      -- notes: none
      ---------------------------------------------------------------------------
      procedure MOVE_RING( FROM, TO : POLE_NO ) is
	 R : RING_NO := POLES( FROM ).RINGS( POLES( FROM).RING_HEIGHT);
      begin
	 
	LIFT_RING : declare
            OLD_H : constant HEIGHT := POLES( FROM ).RING_HEIGHT;
        begin
	    for H in OLD_H..POLE_HEIGHT loop
	    	DRAW_RING( FROM, H, RING_NO'( 0 ) );
	   	exit when H = POLE_HEIGHT;
	   	DRAW_RING( FROM, H + 1, R );
            end loop;
            POLES( FROM ).RINGS( OLD_H ) := RING_NO'( 0 );
            POLES( FROM ).RING_HEIGHT := OLD_H - 1;
     	end LIFT_RING;
     
	DROP_RING: declare
     	    NEW_H : constant HEIGHT := POLES(TO).RING_HEIGHT + 1;
     	begin
	    DRAW_RING(TO, POLE_HEIGHT, R);
    	    for H in reverse NEW_H .. POLE_HEIGHT - 1 loop
       	    	DRAW_RING(TO, H + 1, RING_NO'(0));
       	    	DRAW_RING(TO, H, R);
    	    end loop;
    	    POLES(TO).RING_HEIGHT := NEW_H;
    	    POLES(TO).RINGS(NEW_H) := R;
 	end DROP_RING;
     end MOVE_RING;
      
   begin  -- SOLVE
      if R > 0 then
	 declare
	    OTHER_POLE : constant POLE_NO := 6 - START_POLE - FINISH_POLE;
	 begin
	    SOLVE(R - 1, START_POLE => START_POLE, FINISH_POLE => OTHER_POLE);
	    MOVE_RING(FROM => START_POLE, TO => FINISH_POLE );
	    SOLVE(R - 1, START_POLE => OTHER_POLE, FINISH_POLE => FINISH_POLE);
	 end;
      end if;
   end SOLVE;

   ----------------------------
   -- Start of main program
   ----------------------------

   procedure Xgc_Attach_Sch;
   pragma Import(C, Xgc_Attach_Sch, "__xgc_attach_sch");

begin -- HANOI

   XGC_Attach_Sch;  -- for XGC Text_IO 
   
   DRAW_START;
   loop                                                        
      SOLVE(MAX_RING_NO, START_POLE => 1, FINISH_POLE => 3);
      SOLVE(MAX_RING_NO, START_POLE => 3, FINISH_POLE => 1); 
   end loop;                                                    
   Fix_Cursor( 22, 1);
   New_Line;

end HANOI;
