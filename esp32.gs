function doPost(e) {
  const name =
    Utilities.formatDate(new Date(), 'GMT+0', 'yyyyMMdd-HHmmss') + '.jpg'
  const subFolderName = Utilities.formatDate(new Date(), 'GMT+0', 'yyyyMMdd')
  const folderName = e.parameters.folder || 'ESP32-CAM'
  const data = Utilities.base64Decode(e.postData.contents)
  const blob = Utilities.newBlob(data, 'image/jpg', name)

  let folder
  const folders = DriveApp.getFoldersByName(folderName)
  if (folders.hasNext()) {
    folder = folders.next()
  } else {
    folder = DriveApp.createFolder(folderName)
  }
  let subFolder
  const subFolders = folder.getFoldersByName(subFolderName)
  if (subFolders.hasNext()) {
    subFolder = subFolders.next()
  } else {
    subFolder = folder.createFolder(subFolderName)
  }
  const file = subFolder.createFile(blob)
  return ContentService.createTextOutput('Done')
}
