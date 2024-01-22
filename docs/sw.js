
var base = '/ESP32_Camper';
if (location.href.startsWith('http://localhost')) {
  base = '/docs';
}
var GHPATH = base;
var APP_PREFIX = 'esp32_camper_';
var VERSION = 'version_024';
var URLS = [
  `${GHPATH}/`,
  `${GHPATH}/index.html`,
  `${GHPATH}/css/styles.css`,
  `${GHPATH}/js/app.js`,
  `${GHPATH}/img/truck.png`,
  `${GHPATH}/img/autoplay.svg`,
  `${GHPATH}/img/bluetooth_connected.svg`,
  `${GHPATH}/img/bluetooth_disabled.svg`,
  `${GHPATH}/img/bolt.svg`,
  `${GHPATH}/img/heat.svg`,
  `${GHPATH}/img/humidity_percentage.svg`,
  `${GHPATH}/img/mode_fan.svg`,
  `${GHPATH}/img/power.svg`,
  `${GHPATH}/img/schedule.svg`,
  `${GHPATH}/img/sync.svg`,
  `${GHPATH}/img/thermometer.svg`,
]

var CACHE_NAME = APP_PREFIX + VERSION
self.addEventListener('fetch', function (e) {
  console.log('Fetch request : ' + e.request.url);
  e.respondWith(
    caches.match(e.request).then(function (request) {
      if (request) {
        console.log('Responding with cache : ' + e.request.url);
        return request
      } else {
        console.log('File is not cached, fetching : ' + e.request.url);
        return fetch(e.request)
      }
    })
  )
})

self.addEventListener('install', function (e) {
  e.waitUntil(
    caches.open(CACHE_NAME).then(function (cache) {
      console.log('Installing cache : ' + CACHE_NAME);
      return cache.addAll(URLS)
    })
  )
})

self.addEventListener('activate', function (e) {
  e.waitUntil(
    caches.keys().then(function (keyList) {
      var cacheWhitelist = keyList.filter(function (key) {
        return key.indexOf(APP_PREFIX)
      })
      cacheWhitelist.push(CACHE_NAME);
      return Promise.all(keyList.map(function (key, i) {
        if (cacheWhitelist.indexOf(key) === -1) {
          console.log('Deleting cache : ' + keyList[i]);
          return caches.delete(keyList[i])
        }
      }))
    })
  )
})

var sendNotification = false;
self.addEventListener('message', function (evt) {
  //on android, notifications can be triggered only from service worker
  //we use the postMessage API to get the data from the page and show the notification
  //ATTENTION!!!
  //Web BT CAN'T be accessed from the SW ... so notifications might be late
  console.log('postMessage received', evt.data);

  if (evt.data.type == 'notification') {
    if (sendNotification) {
      registration.showNotification(
        evt.data.title,
        {
          body: evt.data.body,
          icon: evt.data.icon
        }
      );
    }

  }
  if (evt.data.type == 'enableNotification') {
    sendNotification = evt.data.value;
  }

})

self.addEventListener('notificationclick', function (event) {
  let url = self.location.origin + base + "/";
  if (location.href.startsWith('http://localhost')) {
    url += "index.html";
  }
  event.notification.close(); // Android needs explicit close.
  event.waitUntil(
    clients.matchAll({ type: 'window' }).then(windowClients => {
      // Check if there is already a window/tab open with the target URL
      for (var i = 0; i < windowClients.length; i++) {
        var client = windowClients[i];
        // If so, just focus it.
        if (client.url === url && 'focus' in client) {
          return client.focus();
        }
      }
      // If not, then open the target URL in a new window/tab.
      if (clients.openWindow) {
        return clients.openWindow(url);
      }
    })
  );
})