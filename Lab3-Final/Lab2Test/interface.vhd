--Copyright (C) 1991-2004 Altera Corporation
--Any megafunction design, and related net list (encrypted or decrypted),
--support information, device programming or simulation file, and any other
--associated documentation or information provided by Altera or a partner
--under Altera's Megafunction Partnership Program may be used only to
--program PLD devices (but not masked PLD devices) from Altera.  Any other
--use of such megafunction design, net list, support information, device
--programming or simulation file, or any other related documentation or
--information is prohibited for any other purpose, including, but not
--limited to modification, reverse engineering, de-compiling, or use with
--any other silicon devices, unless such use is explicitly licensed under
--a separate agreement with Altera or a megafunction partner.  Title to
--the intellectual property, including patents, copyrights, trademarks,
--trade secrets, or maskworks, embodied in any such megafunction design,
--net list, support information, device programming or simulation file, or
--any other related documentation or information provided by Altera or a
--megafunction partner, remains with Altera, the megafunction partner, or
--their respective licensors.  No other licenses, including any licenses
--needed under any third party's intellectual property, are provided herein.
--Copying or modifying any file, or portion thereof, to which this notice
--is attached violates this copyright.


-- VHDL Custom Instruction Template File for Extended Logic

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity custominstruction is
port(
	signal clk: in std_logic;				-- CPU system clock (always required)
	signal reset: in std_logic;				-- CPU master asynchronous active high reset (always required)
	signal clk_en: in std_logic;				-- Clock-qualifier (always required)
	signal start: in std_logic;				-- Active high signal used to specify that inputs are valid (always required)
	signal done: out std_logic;				-- Active high signal used to notify the CPU that result is valid (required for variable multi-cycle)
	signal n: in std_logic_vector(7 downto 0);		-- N-field selector (always required), modify width to match the number of unique operations within the custom instruction
	signal dataa: in std_logic_vector(31 downto 0);		-- Operand A (always required)
	signal datab: in std_logic_vector(31 downto 0);		-- Operand B (optional)
	signal result: out std_logic_vector(31 downto 0)	-- result (always required)
);
end entity custominstruction;



architecture a_custominstruction of custominstruction is

	component idct_1D 
generic(
	CONST_BITS: integer := 13;
	PASS1_BITS: integer := 2
);
port(
	signal pass : in std_logic;			-- 0: Pass 1; 1: Pass 2
	
	signal i0: in std_logic_vector(15 downto 0);
	signal i1: in std_logic_vector(15 downto 0);
	signal i2: in std_logic_vector(15 downto 0);
	signal i3: in std_logic_vector(15 downto 0);
	signal i4: in std_logic_vector(15 downto 0);
	signal i5: in std_logic_vector(15 downto 0);
	signal i6: in std_logic_vector(15 downto 0);
	signal i7: in std_logic_vector(15 downto 0);
	
	signal o0: out std_logic_vector(15 downto 0);
	signal o1: out std_logic_vector(15 downto 0);
	signal o2: out std_logic_vector(15 downto 0);
	signal o3: out std_logic_vector(15 downto 0);
	signal o4: out std_logic_vector(15 downto 0);
	signal o5: out std_logic_vector(15 downto 0);
	signal o6: out std_logic_vector(15 downto 0);
	signal o7: out std_logic_vector(15 downto 0)
);
end component;

--signal tempInput1 : std_logic_vector(31 downto 0);
--signal tempInput2 : std_logic_vector(31 downto 0);
--signal tempInput3 : std_logic_vector(31 downto 0);
--signal tempInput4 : std_logic_vector(31 downto 0);
signal temp_pass : std_logic;
type idct_1d_vec is array (0 to 7) of std_logic_vector(15 downto 0);
signal temp_input : idct_1d_vec;
signal temp_output : idct_1d_vec;
--signal tempOutput1 : std_logic_vector(31 downto 0);
--signal tempOutput2 : std_logic_vector(31 downto 0);
--signal tempOutput3 : std_logic_vector(31 downto 0);
--signal tempOutput4 : std_logic_vector(31 downto 0);
--signal counter : std_logic_vector(15 downto 0);
signal input_state : std_logic;                       -- init at '0'
signal output_state : unsigned(1 downto 0);   -- init at '000'

signal input_en : std_logic;
signal output_en : std_logic;

begin
  
  input_en <= '1' when n = "00000000" else '0';
  output_en <= '1' when n(1) = '1' else '0';
  
  PROCESS(clk, reset)
  BEGIN
    --if rising_edge(clk) AND reset = '1' THEN
    if reset = '1' THEN
      input_state <= '0';
      output_state <= "00";
    else
      if rising_edge(clk) AND clk_en = '1' AND input_en = '1' THEN
        input_state <= NOT input_state;
      END if;
          
      if rising_edge(clk) AND clk_en = '1' AND output_en = '1' THEN
          output_state <= unsigned( output_state ) + 1;
      END if;
    END if;
  END PROCESS;
  
  
  
  PROCESS(clk, clk_en)
  BEGIN
    if rising_edge(clk) AND clk_en = '1' AND input_en = '1' THEN
      if input_state = '0' THEN
        temp_input(0) <= dataa(31 downto 16);
        temp_input(1) <= dataa(15 downto 0);
        temp_input(2) <= datab(31 downto 16);
        temp_input(3) <= datab(15 downto 0);
      else
        temp_input(4) <= dataa(31 downto 16);
        temp_input(5) <= dataa(15 downto 0);
        temp_input(6) <= datab(31 downto 16);
        temp_input(7) <= datab(15 downto 0);
      END if;
    END if;
  END PROCESS;


  IDCT_1: idct_1D port map (temp_pass,
                            temp_input(0), temp_input(1), temp_input(2), temp_input(3), temp_input(4), temp_input(5), temp_input(6), temp_input(7), 
                            temp_output(0), temp_output(1), temp_output(2), temp_output(3), temp_output(4), temp_output(5), temp_output(6), temp_output(7));

  temp_pass <= n(0);
  
  PROCESS(clk, clk_en)
  BEGIN
    if rising_edge(clk) AND clk_en = '1' AND output_en = '1' THEN
      if(output_state = "00") then
        result <= temp_output(0) & temp_output(1);
      elsif(output_state = "01") then
        result <= temp_output(2) & temp_output(3);
      elsif(output_state = "10") then
        result <= temp_output(4) & temp_output(5);
      else
        result <= temp_output(6) & temp_output(7);
      end if;
    END if;
  END PROCESS;

	-- custom instruction logic (note:  external interfaces can be used as well)

	-- use the n[7..0] port as a select signal on a multiplexer to select the value to feed result[31..0]


end architecture a_custominstruction;

