cd Scripts
$manual="http://www.sci.utah.edu/images/docs/imagevis3d.pdf"
$mdata="http://ci.sci.utah.edu:8011/devbuilds/GettingDataIntoImageVis3D.pdf"
$webClient = New-Object System.Net.WebClient
$webClient.DownloadFile($manual, $pwd.path+"\installer\ImageVis3D.pdf")
$webClient.DownloadFile($mdata, $pwd.path+"\installer\GettingDataIntoImageVis3D.pdf")
