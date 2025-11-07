# PowerShell
Write-Host "Compiling cms.c ..."
gcc -std=c99 -O2 cms.c -o cms.exe

$pass = 0; $fail = 0
function Run-Case {
  param([string]$Name,[string]$InFile)
  Write-Host ""
  Write-Host "=== $Name ==="
  Get-Content $InFile | .\cms.exe | Tee-Object -FilePath "tests\$Name.out" | Out-Null
  $out = Get-Content "tests\$Name.out" -Raw
  $ok = $false
  switch ($Name) {
    "basic" { if ($out -match "(?i)AUTOSAVE is ON" -and $out -match "(?i)successfully inserted" -and $out -match "(?i)Search results" -and $out -match "(?i)successfully updated" -and $out -match "(?i)SUMMARY") {$ok=$true} }
    "delete_undo" { if ($out -match "(?i)successfully deleted" -and $out -match "(?i)UNDO successful" -and $out -match "(?i)record with ID=999001 is found") {$ok=$true} }
    "sort" { if ($out -match "(?i)Here are all the records") {$ok=$true} }
    "find" { if ($out -match "(?i)Search results") {$ok=$true} }
  }
  if ($ok) { Write-Host "[PASS] $Name"; $global:pass++ } else { Write-Host "[FAIL] $Name"; $global:fail++ }
}
Run-Case -Name "basic" -InFile "tests\basic.in"
Run-Case -Name "delete_undo" -InFile "tests\delete_undo.in"
Run-Case -Name "sort" -InFile "tests\sort.in"
Run-Case -Name "find" -InFile "tests\find.in"
Write-Host ""; Write-Host "Passed: $pass  Failed: $fail"
if ($fail -eq 0) { exit 0 } else { exit 1 }
