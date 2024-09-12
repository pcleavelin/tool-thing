const path = require('path')
const express = require('express')
const app = express()
const port = 3000

app.get('/', (req, res) => {
    res.sendFile(__dirname + '/index.html')
})
app.get('/main.js', (req, res) => {
    res.sendFile(__dirname + '/main.js')
})
app.get('/departure-mono.woff', (req, res) => {
    res.sendFile(path.join(__dirname, '../bin/DepartureMono-Regular.woff2'))
})

app.get('/wasm', (req, res) => {
    res.sendFile(path.join(__dirname, '../bin/app.wasm'))
})

app.listen(port, () => {
    console.log(`Example app listening at http://localhost:${port}`)
})
