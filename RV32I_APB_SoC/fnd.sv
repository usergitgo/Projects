`timescale 1ns / 1ps

module FND_Periph (
    // global signals
    input  logic        PCLK,
    input  logic        PRESET,
    // APB Interface Signals
    input  logic [ 2:0] PADDR,
    input  logic        PWRITE,
    input  logic        PENABLE,
    input  logic [31:0] PWDATA,
    input  logic        PSEL,
    output logic [31:0] PRDATA,
    output logic        PREADY,
    // External Port
    output logic [ 3:0] fndcom,
    output logic [ 7:0] fndfont
);
    logic [13:0] fndata;

    APB_SlaveIntf_FND U_APB_SlaveIntf_FND (.*);

    fndcontroller U_fndController (
        .clk(PCLK),
        .reset(PRESET),
        .en(fnden),
        .num(fndata),
        .fndcom(fndcom),
        .fndfont(fndfont)
    );

endmodule

module APB_SlaveIntf_FND (
    // global signals
    input  logic        PCLK,
    input  logic        PRESET,
    // APB Interface Signals
    input  logic [ 2:0] PADDR,
    input  logic        PWRITE,
    input  logic        PENABLE,
    input  logic [31:0] PWDATA,
    input  logic        PSEL,
    output logic [31:0] PRDATA,
    output logic        PREADY,
    // Internal Port
    output logic fnden,
    output logic [13:0] fndata
);
    logic [31:0] slv_reg0, slv_reg1;  //, slv_reg2, slv_reg3; 

    assign fnden  = slv_reg0[0];
    assign fndata = slv_reg1[13:0];

    always_ff @(posedge PCLK, posedge PRESET) begin
        if (PRESET) begin
            slv_reg0 <= 0;
            slv_reg1 <= 0;
            PRDATA   <= 0;
            PREADY   <= 0;
        end else begin
            PREADY <= 1'b0;
            if (PSEL && PENABLE) begin
                PREADY <= 1'b1;
                if (PWRITE) begin
                    case (PADDR[2])
                        1'd0: slv_reg0 <= PWDATA;
                        1'd1: slv_reg1 <= PWDATA;
                    endcase
                end else begin
                    case (PADDR[2])
                        1'd0: PRDATA <= slv_reg0;
                        1'd1: PRDATA <= slv_reg1;
                    endcase
                end
            end
        end
    end
endmodule

/*module FND (
    input  logic [13:0] fnden,
    input  logic [13:0] fndata,
    output logic [13:0] num
);
    genvar i;
    generate
        for (i = 0; i < 14; i++) begin
            assign num[i] = fnden[i] ? fndata[i] : 1'b0;
        end
    endgenerate
endmodule*/

module fndcontroller (
    input logic clk,
    input logic reset,
    input logic en,

    input  logic [13:0] num,
    output logic [ 3:0] fndcom,
    output logic [ 7:0] fndfont
);
    logic tick_1khz;
    logic [1:0] count;
    logic [3:0] digit_1, digit_10, digit_100, digit_1000, digit;

    clk_div_1khz U_CLK_DIV (
        .clk(clk),
        .reset(reset),
        .en(en),
        .tick_1khz(tick_1khz)
    );

    counter_2bit U_COUNT (
        .clk  (clk),
        .reset(reset),
        .tick (tick_1khz),
        .count(count)
    );

    decoder_2x4 U_DEC (
        .en(en),
        .x(count),
        .y(fndcom)
    );

    digitSplitter U_DS (
        .num(num),
        .digit_1(digit_1),
        .digit_10(digit_10),
        .digit_100(digit_100),
        .digit_1000(digit_1000)
    );

    mux_4x1 U_41MUX (
        .sel(count),
        .x0 (digit_1),
        .x1 (digit_10),
        .x2 (digit_100),
        .x3 (digit_1000),
        .y  (digit)
    );

    BCD_TO_FND_DEC U_BCD_FND (
        .bcd(digit),
        .fnd(fndfont)
    );

endmodule

module clk_div_1khz (
    input  logic clk,
    input  logic reset,
    input  logic en,
    output logic tick_1khz
);
    logic [$clog2(100_000)-1:0] div_counter;

    always_ff @(posedge clk, posedge reset) begin
        if (reset) begin
            div_counter <= 0;
            tick_1khz   <= 0;
        end else begin
            if (en) begin
                if (div_counter == 100_000 - 1) begin
                    div_counter <= 0;
                    tick_1khz   <= 1;
                end else begin
                    div_counter <= div_counter + 1;
                    tick_1khz   <= 0;
                end
            end
        end
    end
endmodule

module counter_2bit (
    input logic clk,
    input logic reset,
    input logic tick,
    output logic [1:0] count
);
    always_ff @(posedge clk, posedge reset) begin
        if (reset) begin
            count <= 0;
        end else begin
            if (tick) begin
                count <= count + 1;
            end
        end
    end
endmodule

module decoder_2x4 (
    input logic en,
    input logic [1:0] x,
    output logic [3:0] y
);
    always_comb begin
        if (en) begin
            y = 4'b1111;
            case (x)
                2'b00: y = 4'b1110;
                2'b01: y = 4'b1101;
                2'b10: y = 4'b1011;
                2'b11: y = 4'b0111;
            endcase
        end else begin
            y = 4'b1111;
        end
    end
endmodule

module digitSplitter (
    input  logic [13:0] num,
    output logic [ 3:0] digit_1,
    output logic [ 3:0] digit_10,
    output logic [ 3:0] digit_100,
    output logic [ 3:0] digit_1000
);
    assign digit_1 = num % 10;
    assign digit_10 = num / 10 % 10;
    assign digit_100 = num / 100 % 10;
    assign digit_1000 = num / 1000 % 10;
endmodule

module mux_4x1 (
    input  logic [1:0] sel,
    input  logic [3:0] x0,
    input  logic [3:0] x1,
    input  logic [3:0] x2,
    input  logic [3:0] x3,
    output logic [3:0] y
);
    always_comb begin
        y = 4'b0;
        case (sel)
            2'b00: y = x0;
            2'b01: y = x1;
            2'b10: y = x2;
            2'b11: y = x3;
        endcase
    end
endmodule

module BCD_TO_FND_DEC (
    input  logic [3:0] bcd,
    output logic [7:0] fnd
);
    always_comb begin
        case (bcd)
            4'h0: fnd = 8'hc0;
            4'h1: fnd = 8'hf9;
            4'h2: fnd = 8'ha4;
            4'h3: fnd = 8'hb0;
            4'h4: fnd = 8'h99;
            4'h5: fnd = 8'h92;
            4'h6: fnd = 8'h82;
            4'h7: fnd = 8'hf8;
            4'h8: fnd = 8'h80;
            4'h9: fnd = 8'h90;
            4'ha: fnd = 8'h88;
            4'hb: fnd = 8'h83;
            4'hc: fnd = 8'hc6;
            4'hd: fnd = 8'ha1;
            4'he: fnd = 8'h86;
            4'hf: fnd = 8'h8e;
            default: fnd = 8'hff;
        endcase
    end

endmodule

/*module led_mode(
    input [1:0] sw,
    output reg [3:0] led
    );

    always @(*) begin
        case (sw)
            2'b00: led = 4'b0001;  
            2'b01: led = 4'b0010;  
            2'b10: led = 4'b0100;  
            2'b11: led = 4'b1000;  
        endcase
    end
endmodule*/
