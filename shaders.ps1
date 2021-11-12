Get-ChildItem "$($PSScriptRoot)\shaders" | 
Foreach-Object { 
    Write-Output($_.Name)
    Start-Process -Wait -NoNewWindow -FilePath "C:\VulkanSDK\1.2.189.2\Bin\glslc.exe" -ArgumentList "$($_.FullName) -o $($PSScriptRoot)\compiled_shaders\$($_.Name).spv" 
}