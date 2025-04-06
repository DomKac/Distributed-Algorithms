import csv
from collections import defaultdict

# Ścieżka do pliku wejściowego
input_file = "input/weatherHistory.csv"

# Wczytanie danych i podział według roku
data_by_year = defaultdict(list)

with open(input_file, "r", encoding="utf-8") as csvfile:
    reader = csv.reader(csvfile)
    headers = next(reader)  # Pobierz nagłówki kolumn

    for row in reader:
        date = row[0]  # Zakładamy, że data jest w pierwszej kolumnie
        year = date.split("-")[0]  # Wyciągnięcie roku z daty
        data_by_year[year].append(row)

# Zapisanie danych do osobnych plików CSV
for year, rows in data_by_year.items():
    output_file = f"{year}.csv"
    with open(output_file, "w", encoding="utf-8", newline="") as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(headers)  # Zapisz nagłówki
        writer.writerows(rows)  # Zapisz wiersze
