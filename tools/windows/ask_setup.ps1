# For play.cmd: the window that says the setup must run again (a new version of the game was downloaded, or a
# setup did not finish), with what it would redo (the file tools/setup.py --check wrote, one item per line).
# Exit status: 0 = run the setup, 1 = play anyway, 2 = closed (neither).
param([string]$DueFile)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Windows.Forms, System.Drawing
[System.Windows.Forms.Application]::EnableVisualStyles()

$due = @()
if ($DueFile -and (Test-Path $DueFile)) { $due = @(Get-Content $DueFile | Where-Object { $_.Trim() }) }
$text = "This copy of the game changed since its last setup (a new version?).`r`n" +
        "The setup must run again to bring it up to date. It takes a few minutes.`r`n"
if ($due.Count -gt 0) { $text += "`r`nTo update:`r`n" + (($due | ForEach-Object { "  - $_" }) -join "`r`n") + "`r`n" }
$text += "`r`nWithout it, the game may not work properly."

$form = New-Object System.Windows.Forms.Form
$form.Text = 'Ratchet & Clank - update needed'
$form.FormBorderStyle = 'FixedDialog'
$form.MaximizeBox = $false
$form.MinimizeBox = $false
$form.StartPosition = 'CenterScreen'
$form.AutoScaleMode = 'Dpi'
$form.AutoSize = $true
$form.AutoSizeMode = 'GrowAndShrink'
$form.Padding = New-Object System.Windows.Forms.Padding 16
$form.TopMost = $true

$layout = New-Object System.Windows.Forms.TableLayoutPanel
$layout.AutoSize = $true
$layout.ColumnCount = 1
$layout.RowCount = 2

$label = New-Object System.Windows.Forms.Label
$label.Text = $text
$label.AutoSize = $true
$label.MaximumSize = New-Object System.Drawing.Size 460, 0
$label.Margin = New-Object System.Windows.Forms.Padding 0, 0, 0, 16
$layout.Controls.Add($label, 0, 0)

$buttons = New-Object System.Windows.Forms.FlowLayoutPanel
$buttons.AutoSize = $true
$buttons.FlowDirection = 'RightToLeft'
$buttons.Dock = 'Fill'
$play = New-Object System.Windows.Forms.Button
$play.Text = 'Play anyway'
$play.AutoSize = $true
$play.Padding = New-Object System.Windows.Forms.Padding 8, 2, 8, 2
$play.DialogResult = [System.Windows.Forms.DialogResult]::No
$setup = New-Object System.Windows.Forms.Button
$setup.Text = 'Run the setup'
$setup.AutoSize = $true
$setup.Padding = New-Object System.Windows.Forms.Padding 8, 2, 8, 2
$setup.DialogResult = [System.Windows.Forms.DialogResult]::Yes
$buttons.Controls.Add($play)
$buttons.Controls.Add($setup)
$layout.Controls.Add($buttons, 0, 1)

$form.Controls.Add($layout)
$form.AcceptButton = $setup
$form.ActiveControl = $setup

switch ($form.ShowDialog()) {
    'Yes' { exit 0 }
    'No' { exit 1 }
    default { exit 2 }
}
