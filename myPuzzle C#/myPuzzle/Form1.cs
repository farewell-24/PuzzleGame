using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Windows.Forms;
using Timer = System.Windows.Forms.Timer;

namespace myPuzzle
{
    public partial class Form1 : Form
    {
        // =========全局变量=========
        private int splitCount = 3;           //动态难度3/4/5
        private Image sourceImg;                    //原始大图
        private int pieceW, pieceH;                 //每块拼图宽高
        private List<PictureBox> puzzleList = new List<PictureBox>();
        private PictureBox dragTarget = null;
        private Point dragOffset;

        //记录每块拼图正确图片，用于完成校验
        private List<Bitmap> correctPieceImgs = new List<Bitmap>();

        //挑战模式相关
        private Timer challengeTimer = new Timer();
        private int remainSecond;
        private bool isChallenging = false;
        private int limitSecond = 30;          // 默认限时30秒

        public Form1()
        {
            InitializeComponent();
            challengeTimer.Interval = 1000;
            challengeTimer.Tick += ChallengeTimer_Tick;

            // 难度下拉框初始化
            cboLevel.Items.AddRange(new string[] { "3×3 简单", "4×4 中等", "5×5 困难" });
            cboLevel.SelectedIndex = 0;
        }

        #region 加载图片，切割拼图核心
        /// <summary>
        /// 加载图片，自适应panel大小，切割为 N*N
        /// </summary>
        private void LoadAndSplitImage(Image srcImage)
        {
            //清空旧拼图
            foreach (var pb in puzzleList)
            {
                panelPuzzle.Controls.Remove(pb);
                pb.Dispose();
            }
            puzzleList.Clear();
            correctPieceImgs.Clear();

            sourceImg = srcImage;
            //图片自适应Panel尺寸
            Bitmap bmpSource = new Bitmap(sourceImg, panelPuzzle.Width, panelPuzzle.Height);
            pieceW = panelPuzzle.Width / splitCount;
            pieceH = panelPuzzle.Height / splitCount;

            //循环切割每一小块
            for (int y = 0; y < splitCount; y++)
            {
                for (int x = 0; x < splitCount; x++)
                {
                    Bitmap piece = new Bitmap(pieceW, pieceH);
                    using (Graphics g = Graphics.FromImage(piece))
                    {
                        g.DrawImage(bmpSource,
                            new Rectangle(0, 0, pieceW, pieceH),
                            new Rectangle(x * pieceW, y * pieceH, pieceW, pieceH),
                            GraphicsUnit.Pixel);
                    }
                    correctPieceImgs.Add((Bitmap)piece.Clone());

                    //生成拼图PictureBox
                    PictureBox picBox = new PictureBox();
                    picBox.Size = new Size(pieceW, pieceH);
                    picBox.Image = piece;
                    picBox.SizeMode = PictureBoxSizeMode.StretchImage;
                    picBox.BorderStyle = BorderStyle.FixedSingle;
                    picBox.Location = new Point(x * pieceW, y * pieceH);

                    //绑定拖拽事件
                    picBox.MouseDown += Pic_MouseDown;
                    picBox.MouseMove += Pic_MouseMove;
                    picBox.MouseUp += Pic_MouseUp;

                    puzzleList.Add(picBox);
                    panelPuzzle.Controls.Add(picBox);
                }
            }
            ShufflePuzzle();
        }

        /// <summary>
        /// 打乱拼图顺序
        /// </summary>
        private void ShufflePuzzle()
        {
            Random rand = new Random();
            //随机交换图片内容
            for (int i = 0; i < puzzleList.Count; i++)
            {
                int j = rand.Next(0, puzzleList.Count);
                Image temp = puzzleList[i].Image;
                puzzleList[i].Image = puzzleList[j].Image;
                puzzleList[j].Image = temp;
            }
        }
        #endregion

        #region 鼠标拖拽逻辑
        private void Pic_MouseDown(object sender, MouseEventArgs e)
        {
            //if (isChallenging == false) return;//挑战开始后拖动
            if (e.Button != MouseButtons.Left) return;
            dragTarget = sender as PictureBox;
            dragOffset = e.Location;
            dragTarget.BringToFront();
        }

        private void Pic_MouseMove(object sender, MouseEventArgs e)
        {
            if (dragTarget == null) return;
            Point newPos = dragTarget.PointToScreen(e.Location);
            Point panelPos = panelPuzzle.PointToClient(newPos);
            dragTarget.Location = new Point(panelPos.X - dragOffset.X, panelPos.Y - dragOffset.Y);
        }

        private void Pic_MouseUp(object sender, MouseEventArgs e)
        {
            if (dragTarget == null) return;

            //找到拖拽落点位置最近拼图块
            PictureBox dropBox = FindNearestPiece(dragTarget);
            if (dropBox != null && dropBox != dragTarget)
            {
                //交换两块拼图图片
                Image tempImg = dragTarget.Image;
                dragTarget.Image = dropBox.Image;
                dropBox.Image = tempImg;
            }

            //复位位置到网格
            ResetAllPieceLocation();
            dragTarget = null;

            //校验拼图是否完成
            if (CheckPuzzleComplete())
            {
                OnPuzzleSuccess();
            }
        }

        /// <summary>
        /// 松开鼠标，拼图对齐网格
        /// </summary>
        private void ResetAllPieceLocation()
        {
            int idx = 0;
            for (int y = 0; y < splitCount; y++)
            {
                for (int x = 0; x < splitCount; x++)
                {
                    puzzleList[idx].Location = new Point(x * pieceW, y * pieceH);
                    idx++;
                }
            }
        }

        /// <summary>
        /// 获取鼠标落点最近拼图块
        /// </summary>
        private PictureBox FindNearestPiece(PictureBox movingPb)
        {
            Point center = new Point(movingPb.Left + movingPb.Width / 2, movingPb.Top + movingPb.Height / 2);
            PictureBox nearPb = null;
            int minDis = int.MaxValue;
            foreach (var pb in puzzleList)
            {
                if (pb == movingPb) continue;
                Point pbCenter = new Point(pb.Left + pb.Width / 2, pb.Top + pb.Height / 2);
                int dist = Math.Abs(center.X - pbCenter.X) + Math.Abs(center.Y - pbCenter.Y);
                if (dist < minDis)
                {
                    minDis = dist;
                    nearPb = pb;
                }
            }
            return nearPb;
        }
        #endregion

        #region 拼图完成判断
        /// <summary>
        /// 判断拼图是否全部正确
        /// </summary>
        private bool CheckPuzzleComplete()
        {
            for (int i = 0; i < puzzleList.Count; i++)
            {
                Bitmap current = (Bitmap)puzzleList[i].Image;
                Bitmap correct = correctPieceImgs[i];
                if (!BitmapEqual(current, correct))
                {
                    return false;
                }
            }
            return true;
        }

        /// <summary>
        /// 位图像素对比
        /// </summary>
        private bool BitmapEqual(Bitmap a, Bitmap b)
        {
            if (a.Size != b.Size) return false;
            for (int x = 0; x < a.Width; x++)
            {
                for (int y = 0; y < a.Height; y++)
                {
                    if (a.GetPixel(x, y) != b.GetPixel(x, y))
                        return false;
                }
            }
            return true;
        }

        private void OnPuzzleSuccess()
        {
            challengeTimer.Stop();
            isChallenging = false;
            MessageBox.Show($"挑战成功！剩余时间：{remainSecond}秒", "成功", MessageBoxButtons.OK, MessageBoxIcon.Information);
            lblTimer.Text = "挑战未开始";
        }
        private void OnChallengeFail()
        {
            challengeTimer.Stop();
            isChallenging = false;
            MessageBox.Show("时间到！挑战失败", "失败", MessageBoxButtons.OK, MessageBoxIcon.Error);
            lblTimer.Text = "挑战未开始";
        }

        /// <summary>取消挑战</summary>
        private void CancelChallenge()
        {
            challengeTimer.Stop();
            isChallenging = false;
            lblTimer.Text = "挑战未开始";
            MessageBox.Show("已取消本次挑战", "提示", MessageBoxButtons.OK, MessageBoxIcon.Information);
        }
        #endregion
        private void ChallengeTimer_Tick(object sender, EventArgs e)
        {
            remainSecond--;
            lblTimer.Text = $"剩余时间：{remainSecond} 秒";
            if (remainSecond <= 0)
            {
                OnChallengeFail();
            }
        }

        private void btnStartChallenge_Click(object sender, EventArgs e)
        {
            if (sourceImg == null)
            {
                MessageBox.Show("请先加载图片！");
                return;
            }
            if (isChallenging)
            {
                MessageBox.Show("当前挑战正在进行！");
                return;
            }

            // 读取用户设定限时
            if (!int.TryParse(txtLimitSecond.Text, out limitSecond) || limitSecond <= 0)
            {
                MessageBox.Show("请输入合法的正整数限时！");
                txtLimitSecond.Text = "30";
                return;
            }

            // 重新打乱一次再开始挑战
            ShufflePuzzle();
            remainSecond = limitSecond;
            isChallenging = true;
            lblTimer.Text = $"剩余时间：{remainSecond} 秒";
            challengeTimer.Start();
        }

        //===== 取消挑战按钮事件 =====
        private void btnCancelChallenge_Click(object sender, EventArgs e)
        {
            if (!isChallenging)
            {
                MessageBox.Show("当前没有正在进行的挑战");
                return;
            }
            CancelChallenge();
        }

        #region 按钮事件
        //查看原图
        private void btnShowOrigin_Click(object sender, EventArgs e)
        {
            if (sourceImg == null)
            {
                MessageBox.Show("请先加载图片");
                return;
            }
            FormOrigin frm = new FormOrigin(sourceImg);
            frm.ShowDialog();
        }

        //试玩新图：打开本地图片
        private void btnNewImage_Click(object sender, EventArgs e)
        {
            if (isChallenging)
            {
                MessageBox.Show("挑战进行中，不能更换图片！");
                return;
            }
            OpenFileDialog openDlg = new OpenFileDialog();
            openDlg.Filter = "图片文件|*.jpg;*.png;*.jpeg";
            if (openDlg.ShowDialog() == DialogResult.OK)
            {
                Image img = Image.FromFile(openDlg.FileName);
                LoadAndSplitImage(img);
            }
        }

        //切换图片：读取项目Images文件夹内置图片
        private void btnSwitchImg_Click(object sender, EventArgs e)
        {
            if (isChallenging)
            {
                MessageBox.Show("挑战进行中，不能更换图片！");
                return;
            }
            string imgDir = Path.Combine(Application.StartupPath, "Images");
            if (!Directory.Exists(imgDir))
            {
                MessageBox.Show("请程序目录下创建Images文件夹放入图片");
                return;
            }
            var files = Directory.GetFiles(imgDir, "*.*")
                .Where(f => f.EndsWith(".jpg") || f.EndsWith(".png")).ToList();
            if (files.Count == 0)
            {
                MessageBox.Show("Images文件夹没有图片");
                return;
            }
            Random r = new Random();
            string selectFile = files[r.Next(files.Count)];
            Image img = Image.FromFile(selectFile);
            LoadAndSplitImage(img);
        }

        //图片重排
        private void btnShuffle_Click(object sender, EventArgs e)
        {
            if (isChallenging)
            {
                MessageBox.Show("挑战进行中，禁止重新打乱！");
                return;
            }
            if (sourceImg == null)
            {
                MessageBox.Show("请先加载图片");
                return;
            }
            ShufflePuzzle();
        }

        //难度切换下拉框事件
        private void cboLevel_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (isChallenging)
            {
                MessageBox.Show("挑战进行中不能切换难度！");
                cboLevel.SelectedIndex = Array.FindIndex(cboLevel.Items.Cast<string>().ToArray(), s => s.StartsWith(splitCount.ToString()));
                return;
            }
            switch (cboLevel.SelectedIndex)
            {
                case 0: splitCount = 3; break;
                case 1: splitCount = 4; break;
                case 2: splitCount = 5; break;
            }
            // 如果已经有图片，立刻重新切割
            if (sourceImg != null)
            {
                LoadAndSplitImage(sourceImg);
            }
        }
        #endregion

        //窗体加载默认加载一张内置图片
        private void Form1_Load(object sender, EventArgs e)
        {
            btnSwitchImg_Click(null, null);
        }
    }
}
