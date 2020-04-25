--
-- Copyright: 2013, Technical University of Denmark, DTU Compute
-- Author: Martin Schoeberl (martin@jopdesign.com)
--         Rasmus Bo Soerensen (rasmus@rbscloud.dk)
--         Wolfgang Puffitsch (rasmus@rbscloud.dk)
-- License: Simplified BSD License
--

-- VHDL top level for Patmos in Chisel on Xilinx ml401 board with on-chip memory
--
-- Includes some 'magic' VHDL code to generate a reset after FPGA configuration.
--

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity patmos_top is
    port(
        clk : in  std_logic;
        oUartPins_txd : out std_logic;
        iUartPins_rxd : in  std_logic
    );
end entity patmos_top;

architecture rtl of patmos_top is
    component Patmos is
        port(
            clk             : in  std_logic;
            reset           : in  std_logic;

            io_comConf_M_Cmd        : out std_logic_vector(2 downto 0);
            io_comConf_M_Addr       : out std_logic_vector(31 downto 0);
            io_comConf_M_Data       : out std_logic_vector(31 downto 0);
            io_comConf_M_ByteEn     : out std_logic_vector(3 downto 0);
            io_comConf_M_RespAccept : out std_logic;
            io_comConf_S_Resp       : in std_logic_vector(1 downto 0);
            io_comConf_S_Data       : in std_logic_vector(31 downto 0);
            io_comConf_S_CmdAccept  : in std_logic;

            io_comSpm_M_Cmd         : out std_logic_vector(2 downto 0);
            io_comSpm_M_Addr        : out std_logic_vector(31 downto 0);
            io_comSpm_M_Data        : out std_logic_vector(31 downto 0);
            io_comSpm_M_ByteEn      : out std_logic_vector(3 downto 0);
            io_comSpm_S_Resp        : in std_logic_vector(1 downto 0);
            io_comSpm_S_Data        : in std_logic_vector(31 downto 0);

            io_ledsPins_led : out std_logic_vector(8 downto 0);
            io_uartPins_tx  : out std_logic;
            io_uartPins_rx  : in  std_logic

        );
    end component;

	 COMPONENT dcm
	 PORT(
			CLKIN_IN : IN std_logic;          
			CLKDV_OUT : OUT std_logic;
			CLKIN_IBUFG_OUT : OUT std_logic;
			CLK0_OUT : OUT std_logic
		  );
  	 END COMPONENT;

	 signal clk_int : std_logic;

    -- for generation of internal reset
    signal int_res            : std_logic;
    signal res_reg1, res_reg2 : std_logic;
    signal res_cnt            : unsigned(2 downto 0) := "000"; -- for the simulation

    attribute xilinx_attribute : string;
    attribute xilinx_attribute of res_cnt : signal is "POWER_UP_LEVEL=LOW";

begin
    -- we use a DCM
  	 Inst_dcm: dcm PORT MAP(
		CLKIN_IN => clk,
		CLKDV_OUT => clk_int,
		CLKIN_IBUFG_OUT => open,
		CLK0_OUT => open
	 );
	  
    --
    --  internal reset generation
    --  should include the PLL lock signal
    --
    process(clk_int)
    begin
        if rising_edge(clk_int) then
            if (res_cnt /= "111") then
                res_cnt <= res_cnt + 1;
            end if;
            res_reg1 <= not res_cnt(0) or not res_cnt(1) or not res_cnt(2);
            res_reg2 <= res_reg1;
            int_res  <= res_reg2;
        end if;
    end process;

    comp : Patmos port map(clk_int, int_res,
           open, open, open, open, open,
           (others => '0'), (others => '0'), '0',
           open, open, open, open,
           (others => '0'), (others => '0'),
           open,
           oUartPins_txd, iUartPins_rxd);

end architecture rtl;
