// Google Apps Script -- deploy as a Web App.
// Extensions -> Apps Script in your Google Sheet, paste this, then
// Deploy -> New deployment -> Web app (Execute as: Me, Access: Anyone).

function doPost(e) {
  var sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
  var d = JSON.parse(e.postData.contents);
  sheet.appendRow([new Date(), d.temp, d.low, d.high, d.avg, d.readings]);
  return ContentService.createTextOutput("ok");
}
