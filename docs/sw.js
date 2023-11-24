
var base = '/ESP32_Camper';
if (location.href.startsWith('http://localhost')) {
  base = '/docs';
}
var GHPATH = base;
var APP_PREFIX = 'esp32_camper_';
var VERSION = 'version_015';
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
  `${GHPATH}/img/schedule.svg`,
  `${GHPATH}/img/sync.svg`,
  `${GHPATH}/img/thermometer.svg`
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
