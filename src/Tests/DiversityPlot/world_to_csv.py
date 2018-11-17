from openpyxl import load_workbook

wb = load_workbook('world.xlsx')
print(wb.sheetnames)