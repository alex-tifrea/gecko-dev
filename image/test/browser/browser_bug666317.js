waitForExplicitFinish();

let pageSource =
  '<html><body>' +
    '<img id="testImg" src="' + TESTROOT + 'big.png">' +
  '</body></html>';

let oldDiscardingPref, oldTab, newTab;
let prefBranch = Cc["@mozilla.org/preferences-service;1"]
                   .getService(Ci.nsIPrefService)
                   .getBranch('image.mem.');

function isImgDecoded() {
  let img = content.document.getElementById('testImg');
  img.QueryInterface(Components.interfaces.nsIImageLoadingContent);
  addMessageListener("test666317:isImgDecoded", function(message) {
    let request = img.getRequest(Components.interfaces.nsIImageLoadingContent.CURRENT_REQUEST);
    let result = request.imageStatus & Components.interfaces.imgIRequest.STATUS_FRAME_COMPLETE ? true : false;

    let msg_name = "test666317:isImgDecoded:Answer_Type_" + message.data.test;
    message.target.sendAsyncMessage(msg_name, { result: result });
  });
}

// Ensure that the image is decoded by drawing it to a canvas.
function forceDecodeImg() {
  let doc = gBrowser.getBrowserForTab(newTab).contentWindow.document;
  let img = doc.getElementById('testImg');
  let canvas = doc.createElement('canvas');
  let ctx = canvas.getContext('2d');
  ctx.drawImage(img, 0, 0);
}

function test() {
  // Enable the discarding pref.
  oldDiscardingPref = prefBranch.getBoolPref('discardable');
  prefBranch.setBoolPref('discardable', true);

  // Create and focus a new tab.
  oldTab = gBrowser.selectedTab;
  newTab = gBrowser.addTab('data:text/html,' + pageSource);
  gBrowser.selectedTab = newTab;

  // Run step2 after the tab loads.
  gBrowser.getBrowserForTab(newTab)
          .addEventListener("pageshow", step2 );
}

function step2() {
  let result;
  let mm = gBrowser.getBrowserForTab(newTab).QueryInterface(Ci.nsIFrameLoaderOwner)
           .frameLoader.messageManager;
  mm.loadFrameScript("data:,(" + isImgDecoded.toString() + ")();", false);

  // Check that the image is decoded.
  mm.addMessageListener("test666317:isImgDecoded:Answer_Type_1", function(message) {
    result = message.data.result;
    ok(result, 'Image should initially be decoded.');
  });

  mm.addMessageListener("test666317:isImgDecoded:Answer_Type_2", function(message) {
    result = message.data.result;

    ok(!result, 'Image should be discarded.');

    // And we're done.
    gBrowser.removeTab(newTab);
    prefBranch.setBoolPref('discardable', oldDiscardingPref);
    finish();
  });

  forceDecodeImg();

  mm.sendAsyncMessage("test666317:isImgDecoded",
                      { test: 1 });

  // Focus the old tab, then fire a memory-pressure notification.  This should
  // cause the decoded image in the new tab to be discarded.
  gBrowser.selectedTab = oldTab;
  var os = Cc["@mozilla.org/observer-service;1"]
             .getService(Ci.nsIObserverService);

  os.notifyObservers(null, 'memory-pressure', 'heap-minimize');

  mm.sendAsyncMessage("test666317:isImgDecoded",
                      { test: 2 });
}
