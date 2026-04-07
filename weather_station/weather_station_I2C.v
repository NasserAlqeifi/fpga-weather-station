module weather_station_I2C (
    input clk_clk,  // Clock
    input reset_reset_n,  // Reset

    // I2C Interface (changed to inout)
    inout i2c_i2c_serial_sda_in, 
    inout i2c_i2c_serial_scl_in, 

    // Additional connections
    input down_external_connection_export, 
    input left_external_connection_export, 
    input up_external_connection_export,   
    input right_external_connection_export,

    // Output signals
    output led_green_external_connection_export, 
    output led_red_external_connection_export,  

    // DHT11 Connection
    inout dht11_external_connection_export
);

    // Internal wires for I2C
    wire m_sda_in;
    wire m_scl_in;
    wire m_sda_oe;
    wire m_scl_oe;

    // Bidirectional logic for I2C
    assign i2c_i2c_serial_sda_in = m_sda_oe ? 1'b0 : 1'bz;
    assign m_sda_in = i2c_i2c_serial_sda_in;
    assign i2c_i2c_serial_scl_in = m_scl_oe ? 1'b0 : 1'bz;
    assign m_scl_in = i2c_i2c_serial_scl_in;

    // Nios_I2C instantiation with I2C connections and additional signal connections
   weather_station soc_inst (
        .clk_clk (clk_clk), 
        .reset_reset_n (reset_reset_n), 

        // I2C connections (modified)
        .i2c_i2c_serial_sda_in (m_sda_in), 
        .i2c_i2c_serial_scl_in (m_scl_in), 
        .i2c_i2c_serial_sda_oe (m_sda_oe), 
        .i2c_i2c_serial_scl_oe (m_scl_oe),

        // Additional inputs and outputs
        .down_external_connection_export (down_external_connection_export),
        .left_external_connection_export (left_external_connection_export),
        .up_external_connection_export (up_external_connection_export),
        .right_external_connection_export (right_external_connection_export),
        .led_green_external_connection_export (led_green_external_connection_export),
        .led_red_external_connection_export (led_red_external_connection_export),

        // DHT11 connection
        .dht11_external_connection_export (dht11_external_connection_export)
    );

endmodule
