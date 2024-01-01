module fsm(
    input wire clk,
    input wire rst,
    input wire [31:0] data_in,
    input wire [31:0] alu_1_out,
    output reg [7:0] addr, 
    output reg rst_div,
    output reg bus_r,
    output reg bus_w,
    output reg ena_rom,
    output reg ena_div,
    output reg [31:0] alu_1_a,
    output reg [31:0] alu_1_b,
    output reg [31:0] data_out
);

    reg [4:0] state;
    reg [31:0] nums;
    reg [7:0] i;
    reg [7:0] j;
    reg [1:0] div_takt;

    reg [31:0] xc; // x for the result
    reg [31:0] yc; // y for the result, should be init as 32'b0
    reg [31:0] xi;
    reg [31:0] xj;
    reg [31:0] res_a_2, res_b_2;
    reg [31:0] res_a_3, res_b_3;
    reg [31:0] yi;
    reg [31:0] div;

    reg pipeline_1;
    reg pipeline_2;
    reg pipeline_3;
    reg pipeline_4;

    always@(posedge clk or negedge rst)
    begin
    if (!rst)
            begin
                state       <= 5'd0;
                addr            = 8'b0;
                bus_r           = 1'b0;
                bus_w           = 1'b0;
                ena_rom         = 1'b0;
                ena_div         = 1'b0;
                
                data_out        = 32'hz;

                nums            = 32'h0;
                i               = 8'h0;
                j               = 8'h0;
                div_takt        = 2'd0;

                xi              = 32'h0;
                xj              = 32'h0;
                
                xc              = 32'h0;
                yc              = 32'h0;

                div             = 32'h0;
                yi              = 32'h0;

                // reg for pipeline 2
                res_a_2           = 32'h0;  
                res_b_2           = 32'h0;

                // reg for pipeline 3
                res_a_3           = 32'h1;
                res_b_3           = 32'h1;

                alu_1_a           = 32'h0;
                alu_1_b           = 32'h0;

                rst_div       = 1'b0;

                pipeline_1 = 1'b0;
                pipeline_2 = 1'b0;
                pipeline_3 = 1'b0;
                pipeline_4 = 1'b0;
            end
            else ctl_cycle;
    end

    task ctl_cycle;
    begin
        casex(state)
            5'd0: begin  // init 初始化，所有值给予一个默认值，一般是0
                state           <= 4'd0;
                addr            = 8'b0;
                bus_r           = 1'b0;
                bus_w           = 1'b0;
                ena_rom         = 1'b0;
                ena_div         = 1'b1;  // 触发器1不激活
                data_out        = 32'hz;
                

                nums            = 32'h0;
                i               = 8'h0;
                j               = 8'h0;
                xi              <= 32'h0;
                xj              <= 32'h0;
                div_takt        = 2'd0;

                xc              = 32'h0;
                yc              = 32'h0;

                div             = 32'h0;  // 寄存器储存变量
                yi              = 32'h0;

                // reg for pipeline 2
                res_a_2           = 32'h0;  
                res_b_2           = 32'h0;

                // reg for pipeline 3
                res_a_3           = 32'h1;
                res_b_3           = 32'h1;

                alu_1_a           = 32'h0;
                alu_1_b           = 32'h0;

                rst_div       = 1'b0;
                ena_div         = 1'b0;
                pipeline_1 = 1'b0;
                pipeline_2 = 1'b0;
                pipeline_3 = 1'b0;
                pipeline_4 = 1'b0;

                state <= 5'd1; // 切换状态到1 状态
            end

            5'd1: begin
                bus_r   = 1'b1;
                ena_rom = 1'b1;  // rom和bus_r同时触发 读取 data_rom文件，作为一个list，通过address作为索引即可读取出点值(0开始）。
                addr    = 8'd0;
                state <= 5'd2;  // 切换状态到2 状态
            end

            5'd2: begin   // get nums from data_rom
                nums = data_in; // 读取第0个数 --- nums 5
                addr = 8'd1;  // for xc
                state <= 5'd3;
            end

            5'd3: begin // load xi
                xc = data_in;  // 读取上一步addr = 1的对应list中的值 --- x
                addr  = i + 3; // 重新获取索引 i = 0（第一次）
                if (i != 0) begin
                    j = 0;
                end
                else begin
                    j = 1;
                end
                res_a_3 = 1; // 分子乘法结果初始化
                res_b_3 = 1; // 分母乘法结果初始化
                state <= 5'd4;
            end

            5'd4: begin // j = 0, load xj
                xi = data_in; // 通过4读取list中第5个数 --- x0 // 【投投投】
                addr = j + 3; // first data is nums, No.2 is xc, No.3 is yc
                state <= 5'd5;
                pipeline_1 = 1'b1;
                // xi = data_in; // 通过4读取list中第5个数 --- x1
            end

            5'd5: begin   // PIPE
                if (pipeline_1) // load xj 读取数据，x1，x2，x3，x4 ....
                begin
                    if (i != j + 1) // for [i=nums-1, j=nums-2], [i=nums-2, j=nums-3]
                        j = j + 1;
                    else 
                        j = j + 2;
                    // change from 3 to 4!!!!!! 
                    addr = j + 3;  // [1] j=2, addr=5
                    xj <= data_in; // [1] x2
                    // else pipeline_1 = 1'b0;
                end

                if (pipeline_2) // x - xj, xi - xj 处理分子分母中的减法
                begin                
                    res_a_2 <= xc - xj;
                    res_b_2 <= xi - xj;
                end

                if (pipeline_3) // res_a_3 = res_a_3 * res_a_2, res_b_3 = res_a_3 * res_a*2 处理分子分母中的乘法
                begin
                    res_a_3 <= res_a_3 * res_a_2;
                    res_b_3 <= res_b_3 * res_b_2;
                end

                pipeline_1 <= j < nums ? 1'b1 : 1'b0;
                pipeline_2 <= pipeline_1 ? 1'b1 : 1'b0; // 判断pipe1 是否为1， 如果是1，则 pipe2=1， 如果是0，pipe2=0
                pipeline_3 <= pipeline_2 ? 1'b1 : 1'b0;
                if (pipeline_1 || pipeline_2 || pipeline_3) state <= 5'd5;
                else state <= 5'd6;
            end

            5'd6: begin  // start div 运算res_a_3/res_b_3
                rst_div = 1'b0;
                ena_div = 1'b1;
                alu_1_a = res_a_3;
                alu_1_b = res_b_3;
                state <= 5'd7;
            end

            5'd7: begin 
                if (div_takt == 2'd2) begin
                    state <= 5'd8; // 状态转移 8
                    div_takt = 2'd0;
                    addr = nums + 3 + i; // yi --- y0
                end
                else div_takt = div_takt + 1;
            end

            5'd8: begin // get div result, read yi
                div = alu_1_out; // = res_a_3/res_b_3
                yi = data_in; // y0的值
                rst_div = 1'b0;
                ena_div = 1'b0;
                state <= 5'd9;
            end

            5'd9: begin // multi
                div = div * yi; // 相乘
                state <= 5'd10; 
            end

            // 此时第一个l0已经求出，继续求l1, l2, l3 ......

            5'd10: begin
                yc = div + yc; // 输出值的求和
                i = i + 1; // [1] i=1
                if (i < nums) begin
                    addr = 1;
                    state <= 5'd3;
                end
                else begin
                    state <= 5'd11;
                    bus_r = 1'b0;
                    bus_w = 1'b1; // 写bus开启。准备将结果输出
                end
            end

            5'd11: begin
                data_out = yc;
                addr = 8'd2;
                state <= 5'd12;
            end

            5'd12: begin
                state <= 5'd13;
            end

            default: begin
                state <= 5'd13;
            end
        endcase
    end
    endtask

endmodule


