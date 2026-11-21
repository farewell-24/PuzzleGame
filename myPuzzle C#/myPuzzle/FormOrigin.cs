using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace myPuzzle
{
    public partial class FormOrigin : Form
    {
        public FormOrigin(Image img)
        {
            InitializeComponent();
            picOrigin.Image = img;
            picOrigin.SizeMode = PictureBoxSizeMode.Zoom;
        }

        private void picOrigin_Click(object sender, EventArgs e)
        {

        }
    }
}
