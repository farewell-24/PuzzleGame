namespace myPuzzle
{
    partial class Form1
    {
        /// <summary>
        ///  Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        ///  Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        ///  Required method for Designer support - do not modify
        ///  the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            panelPuzzle = new Panel();
            btnShowOrigin = new Button();
            btnNewImage = new Button();
            btnSwitchImg = new Button();
            btnShuffle = new Button();
            cboLevel = new ComboBox();
            txtLimitSecond = new TextBox();
            lblTimer = new Label();
            btnStartChallenge = new Button();
            btnCancelChallenge = new Button();
            SuspendLayout();
            // 
            // panelPuzzle
            // 
            panelPuzzle.Location = new Point(10, 10);
            panelPuzzle.Name = "panelPuzzle";
            panelPuzzle.Size = new Size(680, 620);
            panelPuzzle.TabIndex = 0;
            // 
            // btnShowOrigin
            // 
            btnShowOrigin.Location = new Point(723, 115);
            btnShowOrigin.Name = "btnShowOrigin";
            btnShowOrigin.Size = new Size(112, 34);
            btnShowOrigin.TabIndex = 1;
            btnShowOrigin.Text = "查看原图";
            btnShowOrigin.UseVisualStyleBackColor = true;
            btnShowOrigin.Click += btnShowOrigin_Click;
            // 
            // btnNewImage
            // 
            btnNewImage.Location = new Point(841, 115);
            btnNewImage.Name = "btnNewImage";
            btnNewImage.Size = new Size(112, 34);
            btnNewImage.TabIndex = 2;
            btnNewImage.Text = "试玩新图";
            btnNewImage.UseVisualStyleBackColor = true;
            btnNewImage.Click += btnNewImage_Click;
            // 
            // btnSwitchImg
            // 
            btnSwitchImg.Location = new Point(723, 155);
            btnSwitchImg.Name = "btnSwitchImg";
            btnSwitchImg.Size = new Size(112, 34);
            btnSwitchImg.TabIndex = 3;
            btnSwitchImg.Text = "切换图片";
            btnSwitchImg.UseVisualStyleBackColor = true;
            btnSwitchImg.Click += btnSwitchImg_Click;
            // 
            // btnShuffle
            // 
            btnShuffle.Location = new Point(841, 155);
            btnShuffle.Name = "btnShuffle";
            btnShuffle.Size = new Size(112, 34);
            btnShuffle.TabIndex = 4;
            btnShuffle.Text = "图片重排";
            btnShuffle.UseVisualStyleBackColor = true;
            btnShuffle.Click += btnShuffle_Click;
            // 
            // cboLevel
            // 
            cboLevel.FormattingEnabled = true;
            cboLevel.Location = new Point(723, 77);
            cboLevel.Name = "cboLevel";
            cboLevel.Size = new Size(230, 32);
            cboLevel.TabIndex = 5;
            cboLevel.SelectedIndexChanged += cboLevel_SelectedIndexChanged;
            // 
            // txtLimitSecond
            // 
            txtLimitSecond.Location = new Point(841, 257);
            txtLimitSecond.Name = "txtLimitSecond";
            txtLimitSecond.Size = new Size(116, 30);
            txtLimitSecond.TabIndex = 6;
            // 
            // lblTimer
            // 
            lblTimer.AutoSize = true;
            lblTimer.Location = new Point(723, 263);
            lblTimer.Name = "lblTimer";
            lblTimer.Size = new Size(100, 24);
            lblTimer.TabIndex = 7;
            lblTimer.Text = "挑战未开始";
            // 
            // btnStartChallenge
            // 
            btnStartChallenge.Location = new Point(723, 210);
            btnStartChallenge.Name = "btnStartChallenge";
            btnStartChallenge.Size = new Size(112, 34);
            btnStartChallenge.TabIndex = 8;
            btnStartChallenge.Text = "开始挑战";
            btnStartChallenge.UseVisualStyleBackColor = true;
            btnStartChallenge.Click += btnStartChallenge_Click;
            // 
            // btnCancelChallenge
            // 
            btnCancelChallenge.Location = new Point(841, 210);
            btnCancelChallenge.Name = "btnCancelChallenge";
            btnCancelChallenge.Size = new Size(112, 34);
            btnCancelChallenge.TabIndex = 9;
            btnCancelChallenge.Text = "取消挑战";
            btnCancelChallenge.UseVisualStyleBackColor = true;
            btnCancelChallenge.Click += btnCancelChallenge_Click;
            // 
            // Form1
            // 
            AutoScaleDimensions = new SizeF(11F, 24F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(994, 624);
            Controls.Add(btnCancelChallenge);
            Controls.Add(btnStartChallenge);
            Controls.Add(lblTimer);
            Controls.Add(txtLimitSecond);
            Controls.Add(cboLevel);
            Controls.Add(btnShuffle);
            Controls.Add(btnSwitchImg);
            Controls.Add(btnNewImage);
            Controls.Add(btnShowOrigin);
            Controls.Add(panelPuzzle);
            Name = "Form1";
            Text = "myPuzzle";
            Load += Form1_Load;
            ResumeLayout(false);
            PerformLayout();
        }

        #endregion

        private Panel panelPuzzle;
        private Button btnShowOrigin;
        private Button btnNewImage;
        private Button btnSwitchImg;
        private Button btnShuffle;
        private ComboBox cboLevel;
        private TextBox txtLimitSecond;
        private Label lblTimer;
        private Button btnStartChallenge;
        private Button btnCancelChallenge;
    }
}
