---------------------------------------------------------------------------
--
--  Filename:
--
--     man.adb
--
--  Author:
--
--     Chris Nettleton, Camberley, 1988
--
--  Description:
--
--     This program draws a Mandelbrot picture centered at location
--     (x0, y0) and with a given magnification. When the magnification
--     is set to 1.0 and the center set to (0.0, 0.0) we see a circle
--     radius 2.0, with internal detail.
--
--  Revision:
--
--     $Id: man.adb,v 1.2 2011/01/17 04:30:56 nettleto Exp $
--
---------------------------------------------------------------------------

with Text_IO;

procedure Man is
   use Text_IO;

   ------------------------
   -- Stuff to configure --
   ------------------------

   --  Coordinates of center of view

   x0 : constant := 0.0;
   y0 : constant := 0.0;

   --  Magnification of view

   Mag : constant := 1.0;

   --  Max iterations in function Mandelbrot

   Limit : constant := 100;

   --  Scale of terminal in cols/lines across circle radius 2

   x_Scale : constant := 100.0;
   y_scale : constant := 60.0;


   ----------------
   -- Mandelbrot --
   ----------------

   --  Repeat squaring C until C lies outside a circle radius 2.0,
   --  or until we hit the repetition limit. Return the number of
   --  repetitions mod Limit.

   function Mandelbrot (Cx, Cy : Float) return Natural is
      Zx : Float := Cx;
      Zy : Float := Cy;
   begin
      for N in 1 .. Limit loop
         declare
            Zxzx : constant Float := Zx * Zx;
            Zyzy : constant Float := Zy * Zy;
         begin
            if Zxzx + Zyzy >= 4.0 then
               return N;
            else
               Zy := 2.0 * Zx * Zy + Cy;
               Zx := Zxzx - Zyzy + Cx;
            end if;
         end;
      end loop;

      return 0;
   end Mandelbrot;
   
   -- 串口输出
   procedure Xgc_Attach_Sch;
   pragma Import(C, Xgc_Attach_Sch, "__xgc_attach_sch");

begin

   XGC_Attach_Sch;

   declare
      xmin : constant := x0 - 2.0 / Mag;
      xmax : constant := x0 + 2.0 / Mag;
      ymin : constant := y0 - 2.0 / Mag;
      ymax : constant := y0 + 2.0 / Mag;

      dx : constant := (xmax - xmin) / x_Scale;
      dy : constant := (ymax - ymin) / y_Scale;

      x, y : Float;
   begin
      
      loop   -- OUT
	 --  Visit each pixel position and compute pixel color
	 
	 y := ymin;

	 while y <= ymax loop
	    
	    x := xmin;
	    
	    while x <= xmax loop
	       declare
		  Cnt : constant Natural := Mandelbrot (x, y);
	       begin
		  
		  --  Paint one pixel, using different characters
		  --  rather than different colours.
		  
		  Put (Character'Val (Character'Pos (' ') + Cnt rem 96));
		  
		  x := x + dx;
	       end;
	    end loop;
	    
	    New_Line;
	    y := y + dy;
	    
	 end loop; -- Inner
      end loop;   -- OUT
   end;

end Man;

