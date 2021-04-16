# Input data

Currently the input CSV data files look something like this:

integer-1|varchar-32|varchar-32|integer-1
-|-|-|-
1|Mitsubishi|Galant|54708
2|Porsche|911|54289
3|Subaru|Forester|47063
4|Mercury|Sable|70813
5|GMC|Yukon XL 1500|43758
6|Suzuki|Kizashi|54328
7|GMC|Yukon XL 1500|93870
8|Mazda|MX-6|92677
9|Volkswagen|CC|56205
10|Toyota|Camry|46123

This is how the [CAR_DATA](../resources/CAR_DATA.csv) file starts. The whole file must be comma separated. The first line in the file is the header row which has to consist of the following types:

- integer 
- varchar
- null
- decimal
- date

The type must be followed by a hyphon "-" and a number showing how long the data element is. For integer, decimal, date only 1 is currently supported but for varchar and null the data type has a variable size. The whole data element always needs to have a size which is divisible by 4. The sizes of the data types can be configured in the [data_config.ini](../resources/data_config.ini) file. This data size modified multiplied with the number given in the .CSV file will show how many integers will the data element take on the interface.

Rest of the data file is one record per one row. Varchar elements can be surrounded by quotation marks in which those will be removed. These are needed if the string has a comma.