namespace myPuzzle
{
    partial class FormOrigin
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
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
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            picOrigin = new PictureBox();
            ((System.ComponentModel.ISupportInitialize)picOrigin).BeginInit();
            SuspendLayout();
            // 
            // picOrigin
            // 
            picOrigin.Dock = DockStyle.Fill;
            picOrigin.Location = new Point(0, 0);
            picOrigin.Name = "picOrigin";
            picOrigin.Size = new Size(800, 450);
            picOrigin.TabIndex = 0;
            picOrigin.TabStop = false;
            picOrigin.Click += picOrigin_Click;
            // 
            // FormOrigin
            // 
            AutoScaleDimensions = new SizeF(11F, 24F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(800, 450);
            Controls.Add(picOrigin);
            Name = "FormOrigin";
            Text = "FormOrigin";
            ((System.ComponentModel.ISupportInitialize)picOrigin).EndInit();
            ResumeLayout(false);
        }

        #endregion

        private PictureBox picOrigin;
    }
}