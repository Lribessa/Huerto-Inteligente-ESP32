function doGet(e) {
  var sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
  var cadenaDatos = e.parameter.datos;
  
  if (cadenaDatos) {
    var datos = cadenaDatos.split(","); 
    
    // --- NUEVA FORMA DE OBTENER LA MÁXIMA ---
    // Pon tus coordenadas aquí (ejemplo de Madrid, cámbialas por las tuyas)
    var lat = latitud; 
    var lng = longitud;
    
    var urlClima = "https://api.open-meteo.com/v1/forecast?latitude=" + lat + "&longitude=" + lng + "&daily=temperature_2m_max&timezone=auto";
    
    var response = UrlFetchApp.fetch(urlClima);
    var json = JSON.parse(response.getContentText());
    var tempMaxima = json.daily.temperature_2m_max[0]; // Extrae la máxima de hoy

    // Añadimos la fila: Fecha, Hora, S1, S2, S3, Temp Máxima
    sheet.appendRow([datos[0], datos[1], datos[2], datos[3], datos[4], tempMaxima]);
    
    return ContentService.createTextOutput("Dato registrado con éxito");
  } else {
    return ContentService.createTextOutput("Error: No se recibieron datos");
  }
}
