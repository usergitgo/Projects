module fft_cbfp_0 #(
    parameter int WIDTH_IN  = 23,
    parameter int WIDTH_OUT = 11,
    parameter int ARRAY_IN  = 16
) (
    input logic clk,
    input logic rst_n,
    input logic i_valid,

    input logic signed [WIDTH_IN-1:0] din_i[0:ARRAY_IN-1],
    input logic signed [WIDTH_IN-1:0] din_q[0:ARRAY_IN-1],

    output logic dout_valid,
    output logic signed [WIDTH_OUT-1:0] dout_i[0:ARRAY_IN-1],
    output logic signed [WIDTH_OUT-1:0] dout_q[0:ARRAY_IN-1],
    output logic [4:0] index_out[0:511]
);

    reg [5:0] cnt;
    reg [5:0] out_cnt;
    reg [4:0] shift_re[0:3];
    reg [4:0] shift_im[0:3];

    wire [4:0] index_reg_re[0:15];
    wire [4:0] index_reg_im[0:15];
    reg signed [WIDTH_IN-1:0] reg_re[0:63];
    reg signed [WIDTH_IN-1:0] reg_im[0:63];
    reg signed [WIDTH_IN-1:0] reg_re_1[0:63];
    reg signed [WIDTH_IN-1:0] reg_im_1[0:63];

    reg signed [WIDTH_IN-12-1:0] r_reg_re[0:63];
    reg signed [WIDTH_IN-12-1:0] r_reg_im[0:63];
    reg out_ready_valid;
    reg out_valid;

    integer i;
    genvar k;

    logic [4:0] min_reg_re_next;
    logic [4:0] min_reg_im_next;

    reg   [4:0] min_reg_re;
    reg   [4:0] min_reg_im;

    always @(posedge clk, negedge rst_n) begin
        if (!rst_n) begin
            for (i = 0; i < 64; i = i + 1) begin
                reg_re[i] <= '0;
                reg_im[i] <= '0;
            end
        end else if (i_valid) begin
            for (i = 0; i < 16; i = i + 1) begin
                reg_re[((cnt%4)*16)+i] <= din_i[i];
                reg_im[((cnt%4)*16)+i] <= din_q[i];
            end
        end
    end

    always @(posedge clk, negedge rst_n) begin
        if (!rst_n) begin
            for (i = 0; i < 64; i = i + 1) begin
                reg_re_1[i] <= '0;
                reg_im_1[i] <= '0;
            end
        end else begin
            reg_re_1 <= reg_re;
            reg_im_1 <= reg_im;
        end
    end

    for (k = 0; k < 16; k = k + 1) begin
        assign index_reg_re[k] = (din_i[k][21] != din_i[k][WIDTH_IN-1]) ? 0 :
                                 (din_i[k][20] != din_i[k][WIDTH_IN-1]) ? 1 :
                                 (din_i[k][19] != din_i[k][WIDTH_IN-1]) ? 2 :
                                 (din_i[k][18] != din_i[k][WIDTH_IN-1]) ? 3 :
                                 (din_i[k][17] != din_i[k][WIDTH_IN-1]) ? 4 :
                                 (din_i[k][16] != din_i[k][WIDTH_IN-1]) ? 5 :
                                 (din_i[k][15] != din_i[k][WIDTH_IN-1]) ? 6 :
                                 (din_i[k][14] != din_i[k][WIDTH_IN-1]) ? 7 :
                                 (din_i[k][13] != din_i[k][WIDTH_IN-1]) ? 8 :
                                 (din_i[k][12] != din_i[k][WIDTH_IN-1]) ? 9 :
                                 (din_i[k][11] != din_i[k][WIDTH_IN-1]) ? 10 : 
                                 (din_i[k][10] != din_i[k][WIDTH_IN-1]) ? 11 : 
                                 (din_i[k][9] != din_i[k][WIDTH_IN-1]) ? 12 : 
                                 (din_i[k][8] != din_i[k][WIDTH_IN-1]) ? 13 : 
                                 (din_i[k][7] != din_i[k][WIDTH_IN-1]) ? 14 : 
                                 (din_i[k][6] != din_i[k][WIDTH_IN-1]) ? 15 : 
                                 (din_i[k][5] != din_i[k][WIDTH_IN-1]) ? 16 : 
                                 (din_i[k][4] != din_i[k][WIDTH_IN-1]) ? 17 : 
                                 (din_i[k][3] != din_i[k][WIDTH_IN-1]) ? 18 : 
                                 (din_i[k][2] != din_i[k][WIDTH_IN-1]) ? 19 : 
                                 (din_i[k][1] != din_i[k][WIDTH_IN-1]) ? 20 : 
                                 (din_i[k][0] != din_i[k][WIDTH_IN-1]) ? 21 :  
                                 22;
    end

    for (k = 0; k < 16; k = k + 1) begin
        assign index_reg_im[k] = (din_q[k][21] != din_q[k][WIDTH_IN-1]) ? 0 :
                                 (din_q[k][20] != din_q[k][WIDTH_IN-1]) ? 1 :
                                 (din_q[k][19] != din_q[k][WIDTH_IN-1]) ? 2 :
                                 (din_q[k][18] != din_q[k][WIDTH_IN-1]) ? 3 :
                                 (din_q[k][17] != din_q[k][WIDTH_IN-1]) ? 4 :
                                 (din_q[k][16] != din_q[k][WIDTH_IN-1]) ? 5 :
                                 (din_q[k][15] != din_q[k][WIDTH_IN-1]) ? 6 :
                                 (din_q[k][14] != din_q[k][WIDTH_IN-1]) ? 7 :
                                 (din_q[k][13] != din_q[k][WIDTH_IN-1]) ? 8 :
                                 (din_q[k][12] != din_q[k][WIDTH_IN-1]) ? 9 :
                                 (din_q[k][11] != din_q[k][WIDTH_IN-1]) ? 10 : 
                                 (din_q[k][10] != din_q[k][WIDTH_IN-1]) ? 11 : 
                                 (din_q[k][9] != din_q[k][WIDTH_IN-1]) ? 12 : 
                                 (din_q[k][8] != din_q[k][WIDTH_IN-1]) ? 13 : 
                                 (din_q[k][7] != din_q[k][WIDTH_IN-1]) ? 14 : 
                                 (din_q[k][6] != din_q[k][WIDTH_IN-1]) ? 15 : 
                                 (din_q[k][5] != din_q[k][WIDTH_IN-1]) ? 16 : 
                                 (din_q[k][4] != din_q[k][WIDTH_IN-1]) ? 17 : 
                                 (din_q[k][3] != din_q[k][WIDTH_IN-1]) ? 18 : 
                                 (din_q[k][2] != din_q[k][WIDTH_IN-1]) ? 19 : 
                                 (din_q[k][1] != din_q[k][WIDTH_IN-1]) ? 20 : 
                                 (din_q[k][0] != din_q[k][WIDTH_IN-1]) ? 21 :  
                                 22;
    end

    logic [4:0] level1_re[0:7];
    logic [4:0] level2_re[0:3];
    logic [4:0] level3_re[0:1];
    logic [4:0] min_re;

    logic [4:0] level1_im[0:7];
    logic [4:0] level2_im[0:3];
    logic [4:0] level3_im[0:1];
    logic [4:0] min_im;

    always @(*) begin
        for (i = 0; i < 8; i++) begin
            level1_re[i] = (index_reg_re[2*i] < index_reg_re[2*i+1]) ? index_reg_re[2*i] : index_reg_re[2*i+1];
            level1_im[i] = (index_reg_im[2*i] < index_reg_im[2*i+1]) ? index_reg_im[2*i] : index_reg_im[2*i+1];
        end

        for (i = 0; i < 4; i++) begin
            level2_re[i] = (level1_re[2*i] < level1_re[2*i+1]) ? level1_re[2*i] : level1_re[2*i+1];
            level2_im[i] = (level1_im[2*i] < level1_im[2*i+1]) ? level1_im[2*i] : level1_im[2*i+1];
        end

        for (i = 0; i < 2; i++) begin
            level3_re[i] = (level2_re[2*i] < level2_re[2*i+1]) ? level2_re[2*i] : level2_re[2*i+1];
            level3_im[i] = (level2_im[2*i] < level2_im[2*i+1]) ? level2_im[2*i] : level2_im[2*i+1];
        end

        min_re = (level3_re[0] < level3_re[1]) ? level3_re[0] : level3_re[1];
        min_im = (level3_im[0] < level3_im[1]) ? level3_im[0] : level3_im[1];
    end

    always @(*) begin
        logic [4:0] temp_min_re;
        logic [4:0] temp_min_im;

        min_reg_re_next = min_reg_re;
        min_reg_im_next = min_reg_im;

        if ((cnt % 4) == 0) begin
            temp_min_re = shift_re[0];
            temp_min_im = shift_im[0];
            for (i = 1; i < 4; i = i + 1) begin
                if (shift_re[i] < temp_min_re) temp_min_re = shift_re[i];
                if (shift_im[i] < temp_min_im) temp_min_im = shift_im[i];
            end
            min_reg_re_next = temp_min_re;
            min_reg_im_next = temp_min_im;
        end
    end

    always @(posedge clk, negedge rst_n) begin
        if (!rst_n) begin
            min_reg_re <= '0;
            min_reg_im <= '0;
        end else begin
            min_reg_re <= min_reg_re_next;
            min_reg_im <= min_reg_im_next;
        end
    end


    wire [4:0] w_min_reg_re;
    assign w_min_reg_re = (min_reg_re < min_reg_im) ? min_reg_re : min_reg_im;

    always @(posedge clk, negedge rst_n) begin
        if (!rst_n) begin  
            for (
                int i = 0; i < 4; i++
            ) begin  
                shift_re[i] <= '0;
                shift_im[i] <= '0;
            end
        end else begin
            shift_re[cnt[1:0]] <= min_re;
            shift_im[cnt[1:0]] <= min_im;
        end
    end

    always @(posedge clk, negedge rst_n) begin
        if (!rst_n) begin
            for (i = 0; i < 64; i = i + 1) begin
                r_reg_re[i] <= '0;
                r_reg_im[i] <= '0;
            end
            for (i = 0; i < 512; i = i + 1) begin
                index_out[i] <= '0;
            end
            cnt <= '0;
            out_cnt <= '0;
            out_valid <= 1'b0;
            out_ready_valid <= 1'b0;
            dout_valid <= 1'b0;
        end else begin
            if (i_valid) begin
                if (cnt >= 4) begin
                    out_ready_valid <= 1'b1;
                end
                cnt <= cnt + 1;
            end else begin
                cnt <= 0;
            end
            if (out_ready_valid && (out_cnt < 32)) begin
                for (i = 0; i < 64; i = i + 1) begin
                    r_reg_re[i] <= (reg_re_1[i] <<< w_min_reg_re) >>> 12;
                    r_reg_im[i] <= (reg_im_1[i] <<< w_min_reg_re) >>> 12; 
                end

                if ((out_cnt == 1) || !((out_cnt - 1) % 4)) begin
                    for (
                        i = 0; i < 64; i = i + 1
                    ) begin 
                        index_out[(out_cnt-1)*16+i] <= w_min_reg_re;
                    end
                end

                dout_valid <= 1'b1;

                if (out_cnt >= 31) begin
                    out_valid <= 1'b0;
                end
                out_cnt <= out_cnt + 1;
            end else if (out_cnt >= 32) begin
                for (i = 0; i < 64; i = i + 1) begin  
                    index_out[(out_cnt-4)*16+i] <= w_min_reg_re;
                end
                dout_valid <= out_valid;
                out_ready_valid <= 1'b0;
                out_cnt <= 0;
            end else begin
                dout_valid <= 1'b0;
            end
        end
    end

    for (k = 0; k < 16; k = k + 1) begin
        assign dout_i[k] = r_reg_re[(((out_cnt-1)%4)*16)+k];
        assign dout_q[k] = r_reg_im[(((out_cnt-1)%4)*16)+k];
    end

endmodule
