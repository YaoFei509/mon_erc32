---------------------------------------------------------------------------
--
--  Filename:
--
--    pascal.adb
--
--  Description:
--
--    Test program for functions that return unconstrained arrays (such as
--    strings). This program uses recursion to ensure calls are made to a
--    certain depth, presumably enough to fill the register cache on the
--    ERC32.
--
--  Revision:
--
--    $Id: pascal.adb,v 1.2 2009/08/03 23:43:52 nettleto Exp $
--
---------------------------------------------------------------------------


with XGC.Text_IO; use XGC.Text_IO;

procedure Pascal is

   Page_Width : constant := 100;
   Triangle_Height : constant := 18;


   type Row is array (Natural range <>) of Long_Integer;

   First_Row : Row (1 .. 1) := (1 => 1);
   Second_Row : Row (1 .. 3) := (1, 2, 1);


   -----------------------
   -- Local subprograms --
   -----------------------

   function Row_Image (Input : Row) return String;

   function Rest_New_Row (Input : Row) return Row;

   procedure Put_Triangle (N : Natural; Input : Row);


   ---------------
   -- Row_Image --
   ---------------

   --  Returns Row'Image, for example: 1 2 3 4 5 6

   function Row_Image (Input : Row) return String is
   begin
      if Input'Length = 1 then
         return Long_Integer'Image (Input (Input'First));
      else
         return Long_Integer'Image (Input (Input'First)) &
            Row_Image (Input (Input'First + 1 .. Input'Last));
      end if;
   end Row_Image;

   ------------------
   -- Rest_New_Row --
   ------------------

   --  Adds pairs from the old row to make items in the new row. The
   --  last item (always a 1) is just copied (or added to zero if you
   --  like).

   function Rest_New_Row (Input : Row) return Row is
      subtype Unit_Row is Row (1 .. 1);
   begin
      if Input'Length <= 1 then
         return Input;
      else
         return Unit_Row'(1 => Input (Input'First) + Input (Input'First + 1)) &
                  Rest_New_Row (Input (Input'First + 1 .. Input'Last));
      end if;
   end Rest_New_Row;

   ------------------
   -- Put_Triangle --
   ------------------

   --  Recursively draws the triangle assuming a page width and centering
   --  the triangle horizontally.

   procedure Put_Triangle (N : Natural; Input : Row) is
      IM : constant String := Row_Image (Input);
   begin

      for i in 1 .. (Page_Width - IM'Length) / 2 loop
         Put (' ');
      end loop;

      Put (IM);
      New_Line;

      if Input'Length = 1 then
         Put_Triangle (N - 1, Second_Row);

      elsif N > 0 then
         Put_Triangle (N - 1,
            Input (Input'First .. Input'First) & Rest_New_Row (Input));

      end if;
   end Put_Triangle;
   
begin
   
   loop 
      Put_Triangle (Triangle_Height, First_Row);
   end loop;
   
end Pascal;

