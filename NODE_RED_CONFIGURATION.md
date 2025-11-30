# Node-RED Configuration Guide for Battery Sensors

This guide shows how to configure all battery sensor parameters from Node-RED using Signal K API calls.

## Overview

Your ESP32 firmware exposes these configurable parameters via Signal K PUT requests:

| Parameter | House Battery Path | Starter Battery Path | Type | Unit | Range |
|-----------|-------------------|----------------------|------|------|-------|
| Amp-hours (Ah) | `electrical.batteries.house.ah` | `electrical.batteries.starter.ah` | float | Ah | any |
| Charge Efficiency | `electrical.batteries.house.ah/chargeEfficiency` | `electrical.batteries.starter.ah/chargeEfficiency` | float | % | 0-100 |
| Discharge Efficiency | `electrical.batteries.house.ah/dischargeEfficiency` | `electrical.batteries.starter.ah/dischargeEfficiency` | float | % | 0-100 |

## Node-RED HTTP PUT Request Template

All configuration uses HTTP PUT requests to your Signal K server:

```
POST http://<SIGNALK_SERVER>:3000/signalk/v1/api/vessels/self/<PATH>
Content-Type: application/json
Authorization: Bearer <TOKEN> (if authentication enabled)

{
  "value": <VALUE>
}
```

## Example Node-RED Flows

### 1. Simple Dashboard to Set Ah Value

**Flow Name:** Set Battery Ah

**Nodes:**
1. **UI Number Input** (node-red-dashboard)
   - Label: "House Battery Ah"
   - Min: 0, Max: 200, Step: 1
   - Output: `msg.payload`

2. **Function Node** - Prepare PUT Request
   ```javascript
   msg.url = "http://localhost:3000/signalk/v1/api/vessels/self/electrical/batteries/house/ah";
   msg.method = "PUT";
   msg.payload = {
     "value": msg.payload
   };
   msg.headers = {
     "Content-Type": "application/json"
   };
   return msg;
   ```

3. **HTTP Request Node**
   - Method: Use `msg.method`
   - URL: Use `msg.url`
   - Payload: Send as JSON body

4. **Debug Node** - Verify Response

**Connection:** Input → Function → HTTP Request → Debug

---

### 2. Set Charge Efficiency (Positive Current)

**Flow Name:** Configure Charge Efficiency

**Nodes:**
1. **UI Number Input** (node-red-dashboard)
   - Label: "House Charge Efficiency (%)"
   - Min: 0, Max: 100, Step: 1

2. **Function Node** - Prepare Request
   ```javascript
   msg.url = "http://localhost:3000/signalk/v1/api/vessels/self/electrical/batteries/house/ah/chargeEfficiency";
   msg.method = "PUT";
   msg.payload = {
     "value": msg.payload
   };
   msg.headers = {
     "Content-Type": "application/json"
   };
   return msg;
   ```

3. **HTTP Request Node**
4. **Debug Node**

---

### 3. Set Discharge Efficiency (Negative Current)

**Flow Name:** Configure Discharge Efficiency

**Nodes:**
1. **UI Number Input** (node-red-dashboard)
   - Label: "House Discharge Efficiency (%)"
   - Min: 0, Max: 100, Step: 1

2. **Function Node** - Prepare Request
   ```javascript
   msg.url = "http://localhost:3000/signalk/v1/api/vessels/self/electrical/batteries/house/ah/dischargeEfficiency";
   msg.method = "PUT";
   msg.payload = {
     "value": msg.payload
   };
   msg.headers = {
     "Content-Type": "application/json"
   };
   return msg;
   ```

3. **HTTP Request Node**
4. **Debug Node**

---

### 4. Complete Battery Configuration Dashboard

**Flow Name:** Battery Configuration Panel

**Dashboard Layout:**
```
┌─ House Battery Configuration ────────┐
│                                      │
│  Ah Value:           [____100____]   │
│  [SET]                               │
│                                      │
│  Charge Efficiency:  [____95____] %  │
│  [SET]                               │
│                                      │
│  Discharge Efficiency: [____98____]% │
│  [SET]                               │
│                                      │
└──────────────────────────────────────┘
```

**Node Structure:**

```
[Ah Input] ──→ [Prepare Ah] ──→ [HTTP PUT] ──→ [Response]
                                    ↑
[Charge Eff Input] ──→ [Prepare Charge] ──┘

[Discharge Eff Input] ──→ [Prepare Discharge] ──→ [HTTP PUT] ──→ [Response]
```

---

### 5. Complete Flow Export (Copy-Paste Ready)

Here's a full Node-RED flow you can import:

```json
[
  {
    "id": "battery-config-group",
    "type": "tab",
    "label": "Battery Configuration",
    "disabled": false,
    "info": ""
  },
  {
    "id": "house-ah-input",
    "type": "ui_number",
    "z": "battery-config-group",
    "name": "House Battery Ah",
    "label": "Ah",
    "tooltip": "Set house battery Ah value",
    "group": "house-battery",
    "order": 1,
    "width": 6,
    "height": 1,
    "passthru": true,
    "topic": "house_ah",
    "topicType": "str",
    "min": 0,
    "max": 200,
    "step": 1,
    "format": "{{value}}",
    "x": 100,
    "y": 60,
    "wires": [["house-ah-prepare"]]
  },
  {
    "id": "house-ah-prepare",
    "type": "function",
    "z": "battery-config-group",
    "name": "Prepare Ah PUT",
    "func": "msg.url = \"http://localhost:3000/signalk/v1/api/vessels/self/electrical/batteries/house/ah\";\nmsg.method = \"PUT\";\nmsg.payload = { \"value\": msg.payload };\nmsg.headers = { \"Content-Type\": \"application/json\" };\nreturn msg;",
    "outputs": 1,
    "noerr": 0,
    "initialize": "",
    "finalize": "",
    "libs": [],
    "x": 300,
    "y": 60,
    "wires": [["house-ah-http"]]
  },
  {
    "id": "house-ah-http",
    "type": "http request",
    "z": "battery-config-group",
    "name": "HTTP PUT Request",
    "method": "use_msg_method",
    "ret": "txt",
    "paytoqs": "ignore",
    "url": "",
    "tls": "",
    "persist": false,
    "proxy": "",
    "timeout": 5,
    "x": 500,
    "y": 60,
    "wires": [["house-ah-response"]]
  },
  {
    "id": "house-ah-response",
    "type": "debug",
    "z": "battery-config-group",
    "name": "Response",
    "active": true,
    "tosidebar": true,
    "console": false,
    "tostatus": true,
    "complete": "true",
    "targetType": "full",
    "statusVal": "payload",
    "statusType": "auto",
    "x": 700,
    "y": 60,
    "wires": []
  },
  {
    "id": "house-charge-eff-input",
    "type": "ui_number",
    "z": "battery-config-group",
    "name": "House Charge Efficiency",
    "label": "Charge Efficiency (%)",
    "tooltip": "Efficiency when charging (positive current)",
    "group": "house-battery",
    "order": 2,
    "width": 6,
    "height": 1,
    "passthru": true,
    "topic": "house_charge_eff",
    "topicType": "str",
    "min": 0,
    "max": 100,
    "step": 1,
    "format": "{{value}}",
    "x": 100,
    "y": 140,
    "wires": [["house-charge-eff-prepare"]]
  },
  {
    "id": "house-charge-eff-prepare",
    "type": "function",
    "z": "battery-config-group",
    "name": "Prepare Charge Eff PUT",
    "func": "msg.url = \"http://localhost:3000/signalk/v1/api/vessels/self/electrical/batteries/house/ah/chargeEfficiency\";\nmsg.method = \"PUT\";\nmsg.payload = { \"value\": msg.payload };\nmsg.headers = { \"Content-Type\": \"application/json\" };\nreturn msg;",
    "outputs": 1,
    "noerr": 0,
    "initialize": "",
    "finalize": "",
    "libs": [],
    "x": 300,
    "y": 140,
    "wires": [["house-charge-eff-http"]]
  },
  {
    "id": "house-charge-eff-http",
    "type": "http request",
    "z": "battery-config-group",
    "name": "HTTP PUT Request",
    "method": "use_msg_method",
    "ret": "txt",
    "paytoqs": "ignore",
    "url": "",
    "tls": "",
    "persist": false,
    "proxy": "",
    "timeout": 5,
    "x": 500,
    "y": 140,
    "wires": [["house-charge-eff-response"]]
  },
  {
    "id": "house-charge-eff-response",
    "type": "debug",
    "z": "battery-config-group",
    "name": "Response",
    "active": true,
    "tosidebar": true,
    "console": false,
    "tostatus": true,
    "complete": "true",
    "targetType": "full",
    "statusVal": "payload",
    "statusType": "auto",
    "x": 700,
    "y": 140,
    "wires": []
  },
  {
    "id": "house-discharge-eff-input",
    "type": "ui_number",
    "z": "battery-config-group",
    "name": "House Discharge Efficiency",
    "label": "Discharge Efficiency (%)",
    "tooltip": "Efficiency when discharging (negative current)",
    "group": "house-battery",
    "order": 3,
    "width": 6,
    "height": 1,
    "passthru": true,
    "topic": "house_discharge_eff",
    "topicType": "str",
    "min": 0,
    "max": 100,
    "step": 1,
    "format": "{{value}}",
    "x": 100,
    "y": 220,
    "wires": [["house-discharge-eff-prepare"]]
  },
  {
    "id": "house-discharge-eff-prepare",
    "type": "function",
    "z": "battery-config-group",
    "name": "Prepare Discharge Eff PUT",
    "func": "msg.url = \"http://localhost:3000/signalk/v1/api/vessels/self/electrical/batteries/house/ah/dischargeEfficiency\";\nmsg.method = \"PUT\";\nmsg.payload = { \"value\": msg.payload };\nmsg.headers = { \"Content-Type\": \"application/json\" };\nreturn msg;",
    "outputs": 1,
    "noerr": 0,
    "initialize": "",
    "finalize": "",
    "libs": [],
    "x": 300,
    "y": 220,
    "wires": [["house-discharge-eff-http"]]
  },
  {
    "id": "house-discharge-eff-http",
    "type": "http request",
    "z": "battery-config-group",
    "name": "HTTP PUT Request",
    "method": "use_msg_method",
    "ret": "txt",
    "paytoqs": "ignore",
    "url": "",
    "tls": "",
    "persist": false,
    "proxy": "",
    "timeout": 5,
    "x": 500,
    "y": 220,
    "wires": [["house-discharge-eff-response"]]
  },
  {
    "id": "house-discharge-eff-response",
    "type": "debug",
    "z": "battery-config-group",
    "name": "Response",
    "active": true,
    "tosidebar": true,
    "console": false,
    "tostatus": true,
    "complete": "true",
    "targetType": "full",
    "statusVal": "payload",
    "statusType": "auto",
    "x": 700,
    "y": 220,
    "wires": []
  },
  {
    "id": "house-battery",
    "type": "ui_group",
    "name": "House Battery Configuration",
    "tab": "dashboard",
    "disp": true,
    "width": "12",
    "collapse": false
  },
  {
    "id": "dashboard",
    "type": "ui_tab",
    "name": "Battery Config",
    "icon": "dashboard"
  }
]
```

---

### 6. Advanced: Starter Battery Configuration

For the **Starter Battery**, use these paths:

- **Ah:** `electrical.batteries.starter.ah`
- **Charge Efficiency:** `electrical.batteries.starter.ah/chargeEfficiency`
- **Discharge Efficiency:** `electrical.batteries.starter.ah/dischargeEfficiency`

Simply duplicate the house battery flows and replace:
```javascript
// Change from:
msg.url = "http://localhost:3000/signalk/v1/api/vessels/self/electrical/batteries/house/ah";

// To:
msg.url = "http://localhost:3000/signalk/v1/api/vessels/self/electrical/batteries/starter/ah";
```

---

### 7. Troubleshooting

**"Connection Refused" error:**
- Verify Signal K server is running on `localhost:3000`
- Check your Signal K server URL/port in Node-RED

**"405 Method Not Allowed":**
- Ensure you're using PUT method, not POST
- Check HTTP Request node has `method: "use_msg_method"`

**Values don't persist after reboot:**
- Your firmware currently doesn't save efficiency/Ah to flash memory
- Node-RED flows can re-apply settings on startup (see next section)

---

### 8. Auto-Apply Settings on Node-RED Startup

To restore your settings automatically when Node-RED restarts:

**Function Node:**
```javascript
// This runs once when Node-RED starts
const config = {
    "electrical.batteries.house.ah": 100,
    "electrical.batteries.house.ah/chargeEfficiency": 95,
    "electrical.batteries.house.ah/dischargeEfficiency": 98
};

for (const [path, value] of Object.entries(config)) {
    const msg = {
        url: `http://localhost:3000/signalk/v1/api/vessels/self/${path}`,
        method: "PUT",
        payload: { value: value },
        headers: { "Content-Type": "application/json" }
    };
    node.send(msg);
}
```

Then send this to an **HTTP Request** node configured with:
- Method: `use_msg_method`
- Retry on error: enabled

---

## Quick Reference: API Endpoints

```bash
# Set House Battery Ah
curl -X PUT http://localhost:3000/signalk/v1/api/vessels/self/electrical/batteries/house/ah \
  -H "Content-Type: application/json" \
  -d '{"value": 100}'

# Set House Charge Efficiency
curl -X PUT http://localhost:3000/signalk/v1/api/vessels/self/electrical/batteries/house/ah/chargeEfficiency \
  -H "Content-Type: application/json" \
  -d '{"value": 95}'

# Set House Discharge Efficiency
curl -X PUT http://localhost:3000/signalk/v1/api/vessels/self/electrical/batteries/house/ah/dischargeEfficiency \
  -H "Content-Type: application/json" \
  -d '{"value": 98}'

# Repeat for Starter Battery (replace "house" with "starter")
```

---

## Next Steps

1. **Install node-red-dashboard** (if not already installed)
2. **Copy the flow JSON** into your Node-RED instance
3. **Update the Signal K server URL** if needed
4. **Deploy** and test from the Dashboard tab
5. **(Optional) Add persistence** to your firmware for permanent settings

