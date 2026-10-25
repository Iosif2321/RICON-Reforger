# claude, TOOL_ONLY: off-screen WPF stand-in for the Script Authorization prompt, used only by wb-run-selftest-claude.ps1.
# Shows one window with the given title, one text and the given buttons; a click writes the caption to -ClickFile and
# closes the window. Never activated, placed off-screen, closes itself after -Seconds.
param([string]$Title, [string]$Message, [string]$Buttons = "Yes,Yes to All,No,No to All", [string]$ClickFile, [int]$Seconds = 30)
Add-Type -AssemblyName PresentationFramework
$w = New-Object System.Windows.Window
$w.Title = $Title; $w.Width = 420; $w.Height = 160; $w.Left = -3000; $w.Top = -3000
$w.ShowActivated = $false; $w.ShowInTaskbar = $false; $w.WindowStartupLocation = "Manual"
$panel = New-Object System.Windows.Controls.StackPanel
if ($Message -ne "") { $tb = New-Object System.Windows.Controls.TextBlock; $tb.Text = $Message; [void]$panel.Children.Add($tb) }
foreach ($b in @($Buttons -split "," | Where-Object { $_ -ne "" })) {
	$btn = New-Object System.Windows.Controls.Button
	$btn.Content = $b
	$btn.Add_Click({ param($s, $e) Add-Content -Path $ClickFile -Value ([string]$s.Content); $w.Close() }.GetNewClosure())
	[void]$panel.Children.Add($btn)
}
$w.Content = $panel
$timer = New-Object System.Windows.Threading.DispatcherTimer
$timer.Interval = [TimeSpan]::FromSeconds($Seconds)
$timer.Add_Tick({ $w.Close() }.GetNewClosure())
$timer.Start()
[void]$w.ShowDialog()
