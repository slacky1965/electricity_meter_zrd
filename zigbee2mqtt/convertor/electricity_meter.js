const fz = require('zigbee-herdsman-converters/converters/fromZigbee');
const tz = require('zigbee-herdsman-converters/converters/toZigbee');
const exposes = require('zigbee-herdsman-converters/lib/exposes');
const reporting = require('zigbee-herdsman-converters/lib/reporting');
const ota = require('zigbee-herdsman-converters/lib/ota');
const utils = require('zigbee-herdsman-converters/lib/utils');
const globalStore = require('zigbee-herdsman-converters/lib/store');
const { postfixWithEndpointName, precisionRound } = require('zigbee-herdsman-converters/lib/utils.js') 
const e = exposes.presets;
const ea = exposes.access;

const attrModelPreset = 0xf000;
const attrAddressPreset = 0xf001;
const attrMeasurementPreset = 0xf002;
const attrDateRelease = 0xf003;
const attrModelName = 0xf004;
const attrPasswordPreset = 0xf005;

const switchDeviceModel = ['No Device', 'KASKAD-1-MT (MIRTEK)', 'KASKAD-11-C1', 'MERCURY-206', 'ENERGOMERA-CE102M', 'ENERGOMERA-CE208BY', 'NEVA-MT124', 'NARTIS-100'];

const tzLocal = {
  device_address_config: {
    key: ['device_address_preset'],
    convertSet: async (entity, key, value, meta) => {
      const device_address_preset = parseInt(value, 10);
      await entity.write('seMetering', {[attrAddressPreset]: {value: device_address_preset, type: 0x23}});
      return {readAfterWriteTime: 250, state: {device_address_preset: value}};
    },
  },
  device_model_config: {
    key: ['device_model_preset'],
    convertSet: async (entity, key, rawValue, meta) => {
      const endpoint = meta.device.getEndpoint(1);
      const value = switchDeviceModel.indexOf(rawValue);
      const payloads = {
        device_model_preset: ['seMetering', {[attrModelPreset]: {value, type: 0x30}}],
      };
      await endpoint.write(payloads[key][0], payloads[key][1]);
      return {
        state: {[key]: rawValue},
      };
    },
  },
  device_password_config: {
    key: ['device_password_preset'],
    convertSet: async (entity, key, value, meta) => {
      const device_password_preset = value.toString();
      await entity.write('seMetering', {[attrPasswordPreset]: {value: device_password_preset, type: 0x41}});
      return {readAfterWriteTime: 250, state: {device_password_preset: value}};
    },
  },
  device_measurement_config: {
    key: ['device_measurement_preset'],
    convertSet: async (entity, key, value, meta) => {
      const device_measurement_preset = parseInt(value, 10);
      await entity.write('seMetering', {[attrMeasurementPreset]: {value: device_measurement_preset, type: 0x20}});
      return {readAfterWriteTime: 250, state: {device_measurement_preset: value}};
    },
    convertGet: async (entity, key, meta) => {
      await entity.read('seMetering', [attrMeasurementPreset]);
    },
  },
  device_metering: {
    key:['tariff', 'tariff1', 'tariff2', 'tariff3', 'tariff4', 'tariff_summ'],
    convertGet: async (entity, key, meta) => {
      await entity.read('seMetering', ['currentTier1SummDelivered', 
                                       'currentTier2SummDelivered', 
                                       'currentTier3SummDelivered', 
                                       'currentTier4SummDelivered', 
                                       'currentSummDelivered', 
                                       'multiplier', 
                                       'divisor']);
    },
    convertSet: async (entity, key, value, meta) => {
      return null;
    },
  },
  device_battery_life: {
    key:['battery_life'],
    convertGet: async (entity, key, meta) => {
      await entity.read('seMetering', ['remainingBattLife']);
    },
    convertSet: async (entity, key, value, meta) => {
      return null;
    },
  },
  device_serial_number: {
    key:['serial_number'],
    convertGet: async (entity, key, meta) => {
      await entity.read('seMetering', ['meterSerialNumber']);
    },
    convertSet: async (entity, key, value, meta) => {
      return null;
    },
  },
  device_date_release: {
    key:['date_release'],
    convertGet: async (entity, key, meta) => {
      await entity.read('seMetering', [attrDateRelease]);
    },
    convertSet: async (entity, key, value, meta) => {
      return null;
    },
  },
  device_model_name: {
    key:['model_name'],
    convertGet: async (entity, key, meta) => {
      await entity.read('seMetering', [attrModelName]);
    },
    convertSet: async (entity, key, value, meta) => {
      return null;
    },
  },
  device_voltage: {
    key:['voltage'],
    convertGet: async (entity, key, meta) => {
      await entity.read('haElectricalMeasurement', ['rmsVoltage', 
                                                    'acVoltageMultiplier', 
                                                    'acVoltageDivisor']);
    },
    convertSet: async (entity, key, value, meta) => {
      return null;
    },
  },
  device_current: {
    key:['current'],
    convertGet: async (entity, key, meta) => {
      await entity.read('haElectricalMeasurement', ['instantaneousLineCurrent', 
                                                    'acCurrentMultiplier', 
                                                    'acCurrentDivisor']);
    },
    convertSet: async (entity, key, value, meta) => {
      return null;
    },
  },
  device_power: {
    key:['power'],
    convertGet: async (entity, key, meta) => {
      await entity.read('haElectricalMeasurement', ['apparentPower', 
                                                    'acPowerMultiplier', 
                                                    'acPowerDivisor']);
    },
    convertSet: async (entity, key, value, meta) => {
      return null;
    },
  },
  device_temperature: {
    key:['temperature'],
    convertGet: async (entity, key, meta) => {
      await entity.read('genDeviceTempCfg', ['currentTemperature']);
    },
    convertSet: async (entity, key, value, meta) => {
      return null;
    },
  },
};

let energy_divisor = 1;
let energy_multiplier = 1;
let voltage_divisor = 1;
let voltage_multiplier = 1;
let current_divisor = 1;
let current_multiplier = 1;
let power_divisor = 1;
let power_multiplier = 1;

const fzLocal = {
  tariff1: {
    cluster: 'seMetering',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
      const result = {};
      if (msg.data.hasOwnProperty('currentTier1SummDelivered')) {
        const data = msg.data['currentTier1SummDelivered'];
        result.tariff1 = parseInt(data)/energy_divisor*energy_multiplier;
      }
      return result;
    },
  },
  tariff2: {
    cluster: 'seMetering',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
      const result = {};
      if (msg.data.hasOwnProperty('currentTier2SummDelivered')) {
        const data = msg.data['currentTier2SummDelivered'];
        result.tariff2 = parseInt(data)/energy_divisor*energy_multiplier;
      }
      return result;
    },
  },
  tariff3: {
    cluster: 'seMetering',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
      const result = {};
      if (msg.data.hasOwnProperty('currentTier3SummDelivered')) {
        const data = msg.data['currentTier3SummDelivered'];
        result.tariff3 = parseInt(data)/energy_divisor*energy_multiplier;
      }
      return result;
    },
  },
  tariff4: {
    cluster: 'seMetering',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
      const result = {};
      if (msg.data.hasOwnProperty('currentTier4SummDelivered')) {
        const data = msg.data['currentTier4SummDelivered'];
        result.tariff4 = parseInt(data)/energy_divisor*energy_multiplier;
      }
      return result;
    },
  },
  tariff_summ: {
    cluster: 'seMetering',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
      const result = {};
      if (msg.data.hasOwnProperty('currentSummDelivered')) {
        const data = msg.data['currentSummDelivered'];
        result.tariff_summ = parseInt(data)/energy_divisor*energy_multiplier;
      }
      return result;
    },
  },
  status: {
    cluster: 'seMetering',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
      const result = {};
      if (msg.data.hasOwnProperty('status')) {
        const data = msg.data['status'];
        const value = parseInt(data);
        return {
          battery_low: (value & 1<<1) > 0,
          tamper:      (value & 1<<2) > 0,
        };
      }
		  return result;
    },
  },
  battery_life: {
    cluster: 'seMetering',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
      const result = {};
      if (msg.data.hasOwnProperty('remainingBattLife')) {
        const data = parseInt(msg.data['remainingBattLife']);
        result.battery_life = data;
      }
      return result;
    },
  },
  serial_number: {
    cluster: 'seMetering',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
      const result = {};
      if (msg.data.hasOwnProperty('meterSerialNumber')) {
        const data = msg.data['meterSerialNumber'];
        result.serial_number = data.toString();
      }
      return result;
    },
  },
  device_measurement_preset: {
    cluster: 'seMetering',
    type: ['readResponse'],
    convert: (model, msg, publish, options, meta) => {
      const result = {};
      if (msg.data.hasOwnProperty(attrMeasurementPreset)) {
        const data = parseInt(msg.data[attrMeasurementPreset]);
        result.device_measurement_preset = data;
      }
      return result;
    },
  }, 
  date_release: {
    cluster: 'seMetering',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
      const result = {};
      if (msg.data.hasOwnProperty(attrDateRelease)) {
        const data = msg.data[attrDateRelease];
        result.date_release = data.toString();
      }
      return result;
    },
  },
  model_name: {
    cluster: 'seMetering',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
      const result = {};
      if (msg.data.hasOwnProperty(attrModelName)) {
        const data = msg.data[attrModelName];
        result.model_name = data.toString();
      }
      return result;
    },
  },
  e_divisor: {
    cluster: 'seMetering',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
      const result = {};
      if (msg.data.hasOwnProperty('divisor')) {
        const data = parseInt(msg.data['divisor']);
        energy_divisor = data;
        result.e_divisor = energy_divisor;
      }
      return result;
    },
  },
  e_multiplier: {
    cluster: 'seMetering',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
      const result = {};
      if (msg.data.hasOwnProperty('multiplier')) {
        const data = parseInt(msg.data['multiplier']);
        energy_multiplier = data;
        result.e_multiplier = energy_multiplier;
      }
      return result;
    },
  },
  voltage: {
    cluster: 'haElectricalMeasurement',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
      const result = {};
      if (msg.data.hasOwnProperty('rmsVoltage')) {
        const data = parseInt(msg.data['rmsVoltage']);
        result.voltage = data/voltage_divisor*voltage_multiplier;
        //meta.logger.info('Voltage: ' + data + ', multiplier: ' + voltage_multiplier + ', divisor: ' + voltage_divisor);
      }
      return result;
    },
  },
  v_multiplier: {
    cluster: 'haElectricalMeasurement',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
      const result = {};
      if (msg.data.hasOwnProperty('acVoltageMultiplier')) {
        const data = parseInt(msg.data['acVoltageMultiplier']);
        voltage_multiplier = data;
        result.v_multiplier = voltage_multiplier;
      }
      return result;
    },
  },
  v_divisor: {
    cluster: 'haElectricalMeasurement',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
      const result = {};
      if (msg.data.hasOwnProperty('acVoltageDivisor')) {
        const data = parseInt(msg.data['acVoltageDivisor']);
        voltage_divisor = data;
        result.v_divisor = voltage_divisor;
      }
      return result;
    },
  },
  current: {
    cluster: 'haElectricalMeasurement',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
      const result = {};
      if (msg.data.hasOwnProperty('instantaneousLineCurrent')) {
        const data = parseInt(msg.data['instantaneousLineCurrent']);
        result.current = data/current_divisor*current_multiplier;
      }
      return result;
    },
  },
  c_multiplier: {
    cluster: 'haElectricalMeasurement',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
      const result = {};
      if (msg.data.hasOwnProperty('acCurrentMultiplier')) {
        const data = parseInt(msg.data['acCurrentMultiplier']);
        current_multiplier = data;
        result.c_multiplier = current_multiplier;
      }
      return result;
    },
  },
  c_divisor: {
    cluster: 'haElectricalMeasurement',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
      const result = {};
      if (msg.data.hasOwnProperty('acCurrentDivisor')) {
        const data = parseInt(msg.data['acCurrentDivisor']);
        current_divisor = data;
        result.c_divisor = current_divisor;
      }
      return result;
    },
  },
  power: {
    cluster: 'haElectricalMeasurement',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
      const result = {};
      if (msg.data.hasOwnProperty('apparentPower')) {
        const data = parseInt(msg.data['apparentPower']);
        result.power = data/power_divisor*power_multiplier;
      }
      return result;
    },
  },
  p_multiplier: {
    cluster: 'haElectricalMeasurement',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
      const result = {};
      if (msg.data.hasOwnProperty('acPowerMultiplier')) {
        const data = parseInt(msg.data['acPowerMultiplier']);
        power_multiplier = data;
        result.p_multiplier = power_multiplier;
      }
      return result;
    },
  },
  p_divisor: {
    cluster: 'haElectricalMeasurement',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
      const result = {};
      if (msg.data.hasOwnProperty('acPowerDivisor')) {
        const data = parseInt(msg.data['acPowerDivisor']);
        power_divisor = data;
        result.p_divisor = power_divisor;
      }
      return result;
    },
  },
  temperature: {
    cluster: 'genDeviceTempCfg',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
      const result = {};
      if (msg.data.hasOwnProperty('currentTemperature')) {
        const data = parseInt(msg.data['currentTemperature']);
        result.temperature = data;
		  }
      return result;
    },
  },
}

const fz_tamper = {
    cluster: 'seMetering',
    type: ['attributeReport', 'readResponse'],
    convert: (model, msg, publish, options, meta) => {
      const result = {};
      if (msg.data.hasOwnProperty('status')) {
        const data = msg.data['status'];
        const value = parseInt(data);
        return {
          battery_low: (value & 1<<1) > 0,
          tamper:      (value & 1<<2) > 0,
        };
      }
      return result;
    },
};

const definition = {
    zigbeeModel: ['Electricity_meter_B85'], // The model ID from: Device with modelID 'lumi.sens' is not supported.
    model: 'Electricity Meter TLSR8258', // Vendor model number, look on the device for a model number
    vendor: 'Slacky-DIY', // Vendor of the device (only used for documentation and startup logging)
    description: 'Electricity Meter', // Description of the device, copy from vendor site. (only used for documentation and startup logging)
    icon: 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAANcAAADXCAYAAACJfcS1AAAACXBIWXMAAAsSAAALEgHS3X78AAAgAElEQVR4nOy9ebxl11Hf+61aa+99zr19bw/qbqlbkzXLluRRBskYW55wDLaxGUwYX2LwS7AJ4PBCSDCJ85gSzJQQkwcJIYHA44GNIWYwBhvbYFseZHmWjKypJbVaPfcdzjl777VWvT9qn9Mtx7wQeGi89fnI7r739rnn7L1qV9WvfvUr2LIt27It27It27It27It27It27It27It27It27It27It27It27It27It27It27It27It27It27It27It27It27It27It27It27It27It27It27It27It27It27It27It27Ite3SbPNxv4LFiIoKqQilSMAVEEVAsZyuAPZTvJ4T5rVWgLP5cSln8jNlD+pYed7blXH9DE5H5IQ3Dl/IX/zFRM/ti3/sr/x4AVXCHQVR1fv8kpSJKIZ92GDvjvy/6koCEEERVgVJKwXLOWx73/5NtOdff3AQQVnaUJ1x6EU+94uKzr7pw/1Mk6O7b7z4c3vmxj9117JbPfRhoQwhhfogBYlRK8XvgB/xBrwkgpSTMRIaIMz/4f5uRUGKsggillGQ5b4W3v65tOdff1LYt6Q/9wPeVS48ffNHaRz/42pN33XXD9iw7ZmlqodnB/mc/Vx645srPfu8bf+af0s5+Dw87Zx7Y/+XDu2vXLuqqXnrlV79o7/Gjp7ZfdvlFu194w7POWlvbXLn0CeftXVnZvee8i849C7VzbaLL0mRlFIw22Prx+6xpqqS63H7mM7eu3fSpT91z40c+8rEPffjmz37y05+7Czj6Bb9Oq6qSkpJls/JF3s6W/SW25Vx/DRPcI8655DJ+5vtew4E3/+zP1Xcf/K6lKthsluW77/tMuenf/px98P/6JdpjvRxMaP2Nr+I3P3XLd93x6U+9maEQ2rtXOLVWEbWKL3zhdXH3rtU4XQ8jKOOnXfv0bTt2xD37955z9oljk31f8aJnn1NX9e6V5W2rumP7fibTCwiy19pu3M06Tq1vUHJm1vX0/YyNjZY29dRRydko2euwpaUlUjKqCnbsWGHXzp00KyvQ1NB2Rz//uU/f/Eu/+n+/813vff97P/KRT90CbJzx0TWEIDnnv3Z6+3iyLef661v4+Z/4l/noT/67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',
    fromZigbee: [fzLocal.e_divisor, fzLocal.e_multiplier, fzLocal.status,
                 fzLocal.tariff1, fzLocal.tariff2, fzLocal.tariff3, fzLocal.tariff4,  fzLocal.tariff_summ,
                 fzLocal.serial_number, fzLocal.date_release, fzLocal.model_name, fzLocal.battery_life,
                 fzLocal.v_multiplier, fzLocal.v_divisor, fzLocal.voltage,
                 fzLocal.c_multiplier, fzLocal.c_divisor, fzLocal.current,
                 fzLocal.p_multiplier, fzLocal.p_divisor, fzLocal.power,
                 fzLocal.temperature], // We will add this later
    toZigbee: [tzLocal.device_address_config, tzLocal.device_model_config, tzLocal.device_password_config, tzLocal.device_measurement_config, 
               tzLocal.device_metering, tzLocal.device_battery_life, tzLocal.device_serial_number, tzLocal.device_date_release,
               tzLocal.device_model_name, tzLocal.device_voltage, tzLocal.device_current, tzLocal.device_power, tzLocal.device_temperature
               ],
    meta: {
//        multiEndpoint: true
    },
    configure: async (device, coordinatorEndpoint, logger) => {
      const firstEndpoint = device.getEndpoint(1);
      await firstEndpoint.read('seMetering', ['remainingBattLife', 'status', attrMeasurementPreset]);
      await firstEndpoint.read('seMetering', ['divisor']);
      await firstEndpoint.read('seMetering', ['multiplier']);
      await firstEndpoint.read('seMetering', ['currentTier1SummDelivered']);
      await firstEndpoint.read('seMetering', ['currentTier2SummDelivered']);
      await firstEndpoint.read('seMetering', ['currentTier3SummDelivered']);
      await firstEndpoint.read('seMetering', ['currentTier4SummDelivered']);
      await firstEndpoint.read('seMetering', ['currentSummDelivered']);
      await firstEndpoint.read('seMetering', ['meterSerialNumber']);
      await firstEndpoint.read('seMetering', [attrMeasurementPreset]);
      await firstEndpoint.read('seMetering', [attrModelName]);
      await firstEndpoint.read('haElectricalMeasurement', ['acVoltageDivisor']);
      await firstEndpoint.read('haElectricalMeasurement', ['acVoltageMultiplier']);
      await firstEndpoint.read('haElectricalMeasurement', ['rmsVoltage']);
      await firstEndpoint.read('haElectricalMeasurement', ['acCurrentDivisor']);
      await firstEndpoint.read('haElectricalMeasurement', ['acCurrentMultiplier']);
      await firstEndpoint.read('haElectricalMeasurement', ['instantaneousLineCurrent']);
      await firstEndpoint.read('haElectricalMeasurement', ['acPowerDivisor']);
      await firstEndpoint.read('haElectricalMeasurement', ['acPowerMultiplier']);
      await firstEndpoint.read('haElectricalMeasurement', ['apparentPower']);
      await reporting.bind(firstEndpoint, coordinatorEndpoint, ['seMetering', 'haElectricalMeasurement', 'genDeviceTempCfg']);
      const payload_tariff1 = [{attribute: {ID: 0x0100, type: 0x25}, minimumReportInterval: 0, maximumReportInterval: 300, reportableChange: 0}];
      await firstEndpoint.configureReporting('seMetering', payload_tariff1);
      const payload_tariff2 = [{attribute: {ID: 0x0102, type: 0x25}, minimumReportInterval: 0, maximumReportInterval: 300, reportableChange: 0}];
      await firstEndpoint.configureReporting('seMetering', payload_tariff2);
      const payload_tariff3 = [{attribute: {ID: 0x0104, type: 0x25}, minimumReportInterval: 0, maximumReportInterval: 300, reportableChange: 0}];
      await firstEndpoint.configureReporting('seMetering', payload_tariff3);
      const payload_tariff4 = [{attribute: {ID: 0x0106, type: 0x25}, minimumReportInterval: 0, maximumReportInterval: 300, reportableChange: 0}];
      await firstEndpoint.configureReporting('seMetering', payload_tariff4);
      await reporting.currentSummDelivered(firstEndpoint, {min: 0, max: 300, change: 0});
      const payload_status = [{attribute: {ID: 0x0200, type: 0x18}, minimumReportInterval: 0, maximumReportInterval: 300, reportableChange: 0}];
      await firstEndpoint.configureReporting('seMetering', payload_status);
      const payload_battery_life = [{attribute: {ID: 0x0201, type: 0x20}, minimumReportInterval: 0, maximumReportInterval: 300, reportableChange: 0}];
      await firstEndpoint.configureReporting('seMetering', payload_battery_life);
      const payload_serial_number = [{attribute: {ID: 0x0308, type: 0x41}, minimumReportInterval: 0, maximumReportInterval: 300, reportableChange: 0}];
      await firstEndpoint.configureReporting('seMetering', payload_serial_number);
      const payload_date_release = [{attribute: {ID: [attrDateRelease], type: 0x41}, minimumReportInterval: 0, maximumReportInterval: 300, reportableChange: 0}];
      await firstEndpoint.configureReporting('seMetering', payload_date_release);
      const payload_model_name = [{attribute: {ID: [attrModelName], type: 0x41}, minimumReportInterval: 0, maximumReportInterval: 300, reportableChange: 0}];
      await firstEndpoint.configureReporting('seMetering', payload_model_name);
      await reporting.rmsVoltage(firstEndpoint, {min: 0, max: 300, change: 0});
      const payload_current = [{attribute: {ID: 0x0501, type: 0x21}, minimumReportInterval: 0, maximumReportInterval: 300, reportableChange: 0}];
      await firstEndpoint.configureReporting('haElectricalMeasurement', payload_current);
      await reporting.apparentPower(firstEndpoint, {min: 0, max: 300, change: 0});
      const payload_temperature = [{attribute: {ID: 0x0000, type: 0x29}, minimumReportInterval: 0, maximumReportInterval: 300, reportableChange: 0}];
      await firstEndpoint.configureReporting('genDeviceTempCfg', payload_temperature);
    },
// Should be empty, unless device can be controlled (e.g. lights, switches).
    exposes: [
      exposes.numeric('tariff1', ea.STATE_GET).withUnit('kWh').withDescription('Tariff 1'),
      exposes.numeric('tariff2', ea.STATE_GET).withUnit('kWh').withDescription('Tariff 2'),
      exposes.numeric('tariff3', ea.STATE_GET).withUnit('kWh').withDescription('Tariff 3'),
      exposes.numeric('tariff4', ea.STATE_GET).withUnit('kWh').withDescription('Tariff 4'),
      exposes.numeric('tariff_summ', ea.STATE_GET).withUnit('kWh').withDescription('Tariff Summation'),
      exposes.text('model_name', ea.STATE_GET).withDescription('Meter Model Name'),
      exposes.text('serial_number', ea.STATE_GET).withDescription('Meter Serial Number'),
      exposes.text('date_release', ea.STATE_GET).withDescription('Meter Date Release'),
      exposes.numeric('voltage', ea.STATE_GET).withUnit('V').withDescription('Voltage'),
      exposes.numeric('current', ea.STATE_GET).withUnit('A').withDescription('Current'),
      exposes.numeric('power', ea.STATE_GET).withUnit('kW').withDescription('Power'),
      exposes.numeric('battery_life', ea.STATE_GET).withUnit('%').withDescription('Battery Life'),
      exposes.numeric('temperature', ea.STATE_GET).withUnit('°C').withDescription('Device temperature'),
      exposes.numeric('device_address_preset', ea.STATE_SET).withDescription('Device Address'),
      exposes.enum('device_model_preset', ea.STATE_SET, switchDeviceModel).withDescription('Device Model'),
      exposes.text('device_password_preset', ea.STATE_SET).withDescription('Meter Password'),
      exposes.numeric('device_measurement_preset', ea.ALL).withDescription('Measurement Period').withValueMin(1).withValueMax(255),
      exposes.binary('tamper', ea.STATE, true, false).withDescription('Tamper'),
      exposes.binary('battery_low', ea.STATE, true, false).withDescription('Battery Low'),
    ],
              
    ota: true,
};

module.exports = definition;


      // exposes.numeric('e_divisor', ea.STATE_GET).withDescription('Divisor of energy'),
      // exposes.numeric('e_multiplier', ea.STATE_GET).withDescription('Multiplier of energy'),
      // exposes.numeric('v_divisor', ea.STATE_GET).withDescription('Divisor of voltage'),
      // exposes.numeric('v_multiplier', ea.STATE_GET).withDescription('Multiplier of voltage'),
      // exposes.numeric('c_divisor', ea.STATE_GET).withDescription('Divisor of current'),
      // exposes.numeric('c_multiplier', ea.STATE_GET).withDescription('Multiplier of current'),
      // exposes.numeric('p_divisor', ea.STATE_GET).withDescription('Divisor of power'),
      // exposes.numeric('p_multiplier', ea.STATE_GET).withDescription('Multiplier of power'),
