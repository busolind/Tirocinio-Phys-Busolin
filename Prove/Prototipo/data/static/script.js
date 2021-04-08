window.onload = () => {
  getRunningConf();
  getSavedConf();
};

var runningConfJSON, savedConfJSON;

function getRunningConf() {
  makeRequest('GET', '/get/running-conf').then((response) => {
    runningConfJSON = JSON.parse(response);
    document.getElementById('json_input').innerHTML = JSON.stringify(runningConfJSON, null, 4);

    document.getElementById('setApiUrl').value = runningConfJSON.apiUrl;
    document.getElementById('setFilterJSON').value = runningConfJSON.filterJSON;
    document.getElementById('setPath').value = runningConfJSON.path;
    document.getElementById('setMinValue').value = runningConfJSON.min_value;
    document.getElementById('setMaxValue').value = runningConfJSON.max_value;
    document.getElementById('setMinPwm').value = runningConfJSON.min_pwm;
    document.getElementById('setMaxPwm').value = runningConfJSON.max_pwm;
    document.getElementById('setRequestInterval').value = runningConfJSON.request_interval_ms;
  });
}

function getSavedConf() {
  makeRequest('GET', '/get/saved-conf').then((response) => {
    savedConfJSON = JSON.parse(response);
    document.getElementById('saved-conf').innerHTML = JSON.stringify(savedConfJSON, null, 4);
  });
}

// From https://stackoverflow.com/a/30008115
function makeRequest(method, url) {
  return new Promise(function (resolve, reject) {
    var xhr = new XMLHttpRequest();
    xhr.open(method, url);
    xhr.onload = function () {
      if (this.status >= 200 && this.status < 300) {
        resolve(xhr.response);
      } else {
        reject({
          status: this.status,
          statusText: xhr.statusText,
        });
      }
    };
    xhr.onerror = function () {
      reject({
        status: this.status,
        statusText: xhr.statusText,
      });
    };
    xhr.send();
  });
}

function sendConfigForm() {
  var fd = new FormData();
  var obj = {};
  Object.assign(obj, { apiUrl: document.getElementById('setApiUrl').value });
  Object.assign(obj, { filterJSON: document.getElementById('setFilterJSON').value });
  Object.assign(obj, { path: document.getElementById('setPath').value });
  Object.assign(obj, { min_value: document.getElementById('setMinValue').value });
  Object.assign(obj, { max_value: document.getElementById('setMaxValue').value });
  Object.assign(obj, { min_pwm: document.getElementById('setMinPwm').value });
  Object.assign(obj, { max_pwm: document.getElementById('setMaxPwm').value });
  Object.assign(obj, { request_interval_ms: document.getElementById('setRequestInterval').value });
  console.log(JSON.stringify(obj));
  fd.append('setFromJSON', JSON.stringify(obj));
  if (document.getElementById('ConfFormSaveToFlash').checked == true) {
    fd.append('saveToFlash', 'on');
    getSavedConf();
  }

  var xhr = new XMLHttpRequest();
  // Define what happens on successful data submission
  xhr.addEventListener('load', function (event) {
    alert('Successfully sent configuration');
  });

  // Define what happens in case of error
  xhr.addEventListener('error', function (event) {
    alert('Oops! Something went wrong.');
  });

  xhr.open('POST', '/post');
  xhr.send(fd);
}
