#include "databasewindow.h"
#include "ui_databasewindow.h"

DataBaseWindow::DataBaseWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::DataBaseWindow)
{
    ui->setupUi(this);
    this->resize(1300, 1000);

    maxImageSize = 250;

    if( QFile::exists(DB_PATH) ){
        msg = "database exist : " + DB_PATH;
        isDBOpened = true;
    }else{
        msg = "No database : " + DB_PATH;
        isDBOpened = false;
        //TODO created a db
        return;
    }

    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(DB_PATH);
    db.open();
    if( !db.isOpen()){
        msg = "Database open Error : " + DB_PATH;
        isDBOpened = false;
        return;
    }

    QStringList tableList = db.tables();
    qDebug() << tableList;

    QStringList dataColName = GetTableColName("Data");
    for(int i = 0; i < dataColName.size() ; i++){
        if( dataColName[i] == "PATH"){
            dataPathCol = i;
            qDebug("data Path Col : %d", i);
            break;
        }
    }

    //===================== set up the sample-table
    sample = new QSqlRelationalTableModel(ui->sampleView);
    SetupSampleTableView();

    connect(ui->sampleView->selectionModel(),
            SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this,
            SLOT(showSamplePicture(QModelIndex))
            );

    //====================== set up the data-table    
    data = new QSqlRelationalTableModel(ui->dataView);
    SetupDataTableView();

    connect(ui->dataView->selectionModel(),
            SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this,
            SLOT(showDataPicture(QModelIndex))
            );

    //====================== Other things
    //ShowTable("Chemical");
    //ShowTable("Sample");
    //ShowTable("Data");

    updateChemicalCombox("Chemical");

    ui->checkBox_sortData->setChecked(true);
    ui->dataView->sortByColumn(2, Qt::DescendingOrder);

    enableChemicalFilter = true;

}

DataBaseWindow::~DataBaseWindow()
{
    delete ui;
}

QStringList DataBaseWindow::GetTableColEntries(QString tableName, int col)
{
    QSqlQuery query;
    query.exec("SELECT *FROM " + tableName);

    QStringList entries;

    while(query.next()){
        entries << query.value(col).toString();
    }

    return entries;
}

int DataBaseWindow::GetTableColNumber(QString tableName)
{
    QSqlQuery query;
    query.exec("SELECT *FROM " + tableName);
    return query.record().count();
}

QStringList DataBaseWindow::GetTableColName(QString tableName)
{
    QSqlQuery query;
    query.exec("SELECT *FROM " + tableName);

    int col = query.record().count();
    QStringList fieldNameList;
    for(int j = 0; j< col; j++){
        fieldNameList << query.record().fieldName(j);
    }
    return fieldNameList;
}

void DataBaseWindow::ShowTable(QString tableName)
{
    QSqlQuery query;
    query.exec("SELECT *FROM " + tableName);

    QString msg, temp;

    int col = query.record().count();
    query.last();
    int row = query.at() + 1;

    msg.sprintf("Table Name : %s, size = %d x %d", tableName.toStdString().c_str(), col, row);
    qDebug() << msg;
    msg.clear();

    QStringList fieldNameList = GetTableColName(tableName);
    qDebug() << fieldNameList;

    query.first();
    query.previous();
    while(query.next()){
        for(int j = 0; j< col; j++){
            fieldNameList << query.record().fieldName(j);
            temp = query.value(j).toString();
            msg += temp;
            msg += " | ";
        }
        qDebug() << msg;
        msg.clear();
    }
}

void DataBaseWindow::SetupSampleTableView()
{
    sample->clear();
    sample->setTable("Sample");
    //sample->setEditStrategy(QSqlTableModel::OnManualSubmit);
    sample->select();

    int chemicalIdx = sample->fieldIndex("Chemical");
    int solventIdx = sample->fieldIndex("Solvent");
    int dateIdx = sample->fieldIndex("Date");
    //int specPathIdx = sample->fieldIndex("SpectrumPath");
    //int picPathIdx = sample->fieldIndex("PicPath");

    //set relation, so that can choose directly on the table
    //sample->setRelation(chemicalIdx, QSqlRelation("Chemical", "NAME", "NAME"));
    //sample->setRelation(solventIdx, QSqlRelation("Solvent", "NAME", "NAME"));

    ui->sampleView->setModel(sample);
    ui->sampleView->resizeColumnsToContents();
    ui->sampleView->setEditTriggers( QAbstractItemView::NoEditTriggers );
    ui->sampleView->setSelectionBehavior( QAbstractItemView::SelectRows );
    ui->sampleView->setSelectionMode(QAbstractItemView::SingleSelection);

    //ui->sampleView->setItemDelegate(new QSqlRelationalDelegate(ui->sampleView));
    //ui->sampleView->setItemDelegateForColumn(dateIdx, new DateFormatDelegate() );
    //ui->sampleView->setItemDelegateForColumn(picPathIdx, new OpenFileDelegate() );
    //ui->sampleView->setItemDelegateForColumn(specPathIdx, new OpenFileDelegate() );
    //ui->sampleView->setColumnHidden(sample->fieldIndex("ID"), true);

    //for some unknown reasons, the column header names are needed to rename;
    sample->setHeaderData(chemicalIdx, Qt::Horizontal, "Chemical");
    sample->setHeaderData(solventIdx, Qt::Horizontal, "Solvent");

    ui->sampleView->setColumnWidth(1, 100);
    ui->sampleView->setColumnWidth(chemicalIdx, 100);
    ui->sampleView->setColumnWidth(solventIdx, 100);
    ui->sampleView->setColumnWidth(dateIdx, 100);

    //connect(ui->pushButton_sumbitSample, SIGNAL(clicked()), this, SLOT(submit()));
}

void DataBaseWindow::SetupDataTableView()
{
    data->clear();
    data->setTable("Data");
    //data->setEditStrategy(QSqlTableModel::OnManualSubmit);

    int sampleIdx = data->fieldIndex("Sample");
    int laserIdx = data->fieldIndex("Laser");
    int dateIdx = data->fieldIndex("Date");
    int repeatIdx = data->fieldIndex("repetition");
    int acummIdx = data->fieldIndex("Average");
    int pointIdx = data->fieldIndex("DataPoint");
    int tempIdx = data->fieldIndex("Temperature");
    int timeRangeIdx = data->fieldIndex("TimeRange");

    data->setHeaderData(laserIdx, Qt::Horizontal, "Laser");
    data->setHeaderData(repeatIdx, Qt::Horizontal, "Repeat.\nRate[Hz]");
    data->setHeaderData(acummIdx, Qt::Horizontal, "Acumm.");
    data->setHeaderData(pointIdx, Qt::Horizontal, "Point");
    data->setHeaderData(tempIdx, Qt::Horizontal, "Temp.\n[K]");
    data->setHeaderData(timeRangeIdx, Qt::Horizontal, "Time\nRange[us]");

    data->select();

    ui->dataView->setModel(data);
    ui->dataView->resizeColumnsToContents();
    ui->dataView->setEditTriggers( QAbstractItemView::NoEditTriggers );
    ui->dataView->setSelectionBehavior( QAbstractItemView::SelectRows );
    if(ui->checkBox_showChemical->isChecked() == false){
        data->setRelation(sampleIdx, QSqlRelation("Sample", "NAME", "NAME"));
        data->setHeaderData(1, Qt::Horizontal, "Sample");
    }else{
        data->setRelation(sampleIdx, QSqlRelation("Sample", "NAME", "Chemical"));
        data->setHeaderData(1, Qt::Horizontal, "Chemcial");
    }

    //data->setRelation(laserIdx, QSqlRelation("Laser", "Name", "Name"));
    //ui->dataView->setItemDelegate(new QSqlRelationalDelegate(ui->sampleView));
    //ui->dataView->setItemDelegateForColumn(dateIdx, new DateFormatDelegate());
    //ui->dataView->setItemDelegateForColumn(dataPathCol, new OpenFileDelegate());

    ui->dataView->setColumnWidth(sampleIdx, 100);
    ui->dataView->setColumnWidth(dateIdx, 100);
    ui->dataView->setColumnWidth(laserIdx, 60);

    if( ui->checkBox_sortData->isChecked()) {
        ui->dataView->sortByColumn(dateIdx, Qt::DescendingOrder);
    }else{
        ui->dataView->sortByColumn(dateIdx, Qt::AscendingOrder);
    }

}

void DataBaseWindow::updateChemicalCombox(QString tableName)
{
    QStringList chemicalList = GetTableColEntries(tableName, sample->fieldIndex("NAME"));
    ui->comboBox_chemical->clear();
    ui->comboBox_chemical->addItem("All");
    ui->comboBox_chemical->addItems(chemicalList);
}

void DataBaseWindow::on_comboBox_chemical_currentTextChanged(const QString &arg1)
{
    if(arg1 == "All") {
        sample->setFilter("");
        data->setFilter("");
        ui->lineEdit_ChemicalFormula->setText("-----");
        ui->label_ChemPicture->clear();
        ui->label_ChemPicture->setText("Chemical Structure");
        return;
    }

    QStringList nameList = GetTableColEntries("Chemical", sample->fieldIndex("NAME"));
    //QStringList formulaList = GetTableColEntries("Chemical", sample->fieldIndex("FORMULA"));
    QStringList formulaList = GetTableColEntries("Chemical", 2); // somehow fieldIndex for FORMULA does not work;
    QStringList picNameList = GetTableColEntries("Chemical", sample->fieldIndex("PicPath"));

    for(int i = 0; i < nameList.size(); i ++ ){
        if( nameList[i] == arg1) {
            ui->lineEdit_ChemicalFormula->setText(formulaList[i]);
            if( !enableChemicalFilter ) break;
            QString picPath = CHEMICAL_PIC_PATH + picNameList[i];
            if( QFile::exists(picPath)){
                QImage image(picPath);
                int imageSize = image.height();
                if( imageSize > maxImageSize) imageSize = maxImageSize;
                QImage scaledImage = image.scaledToHeight(imageSize);
                ui->label_ChemPicture->setPixmap(QPixmap::fromImage(scaledImage));
            }else{
                ui->label_ChemPicture->setText("no image found.\nPlease check the file location.");
            }
            break;
        }
    }

    if( !enableChemicalFilter ) return;
    //select sample
    QString filter = "Chemical='" + arg1 + "'";
    //qDebug() << filter  ;
    sample->setFilter(filter);
    ui->label_SamplePic->clear();
    ui->label_SamplePic->setText("Sample Picture.");

    //showSampleSpectrum(ui->sampleView->model()->index(0,1));
}

void DataBaseWindow::on_pushButton_selectSample_clicked()
{
    QModelIndex current = ui->sampleView->selectionModel()->currentIndex(); // the "current" item
    QString sampleName = sample->index(current.row(), sample->fieldIndex("NAME")).data().toString();

    QString filter = "Sample='" + sampleName + "'";
    qDebug() << filter;
    data->setFilter(filter);
}

void DataBaseWindow::on_pushButton_open_clicked()
{
    QModelIndex current = ui->dataView->selectionModel()->currentIndex(); // the "current" item
    QString dataName = data->index(current.row(), dataPathCol).data().toString();

    //check is it absolute path or relativePath
    QString dataNameFirst = dataName.split("/")[0];

    if( dataNameFirst == "D:"){
        ReturnFilePath(dataName);
    }else{
        ReturnFilePath(DATA_PATH + dataName);
    }

    this->hide();
}

void DataBaseWindow::on_checkBox_sortData_clicked()
{
    SetupDataTableView();
}

void DataBaseWindow::on_checkBox_showChemical_clicked()
{
    SetupDataTableView();
}

void DataBaseWindow::showSamplePicture(const QModelIndex &index)
{
    // find the chemical picture
    QString chemicalName = sample->index(index.row(), sample->fieldIndex("Chemical")).data().toString();
    QSqlQuery query;
    query.exec("SELECT PicPath From Chemical WHERE Chemical.NAME = '" + chemicalName + "'");
    query.last();
    QString chemicalPicPath = CHEMICAL_PIC_PATH + query.value(0).toString();
    if( !QFile::exists(chemicalPicPath) || chemicalPicPath.right(1) == "/"){
        ui->label_ChemPicture->setText("no image found.\nPlease check the file location.");
    }else{
        QImage chemicalPic(chemicalPicPath);
        int imageSize = chemicalPic.height();
        if( imageSize > maxImageSize ) imageSize = maxImageSize;
        QImage scaledChemicalPic = chemicalPic.scaledToHeight(imageSize);
        ui->label_ChemPicture->setPixmap(QPixmap::fromImage(scaledChemicalPic));
    }

    // set the chemical combox
    enableChemicalFilter = false;
    for(int i = 1; i < ui->comboBox_chemical->count(); i++ ){
        if( ui->comboBox_chemical->itemText(i) == chemicalName ){
            ui->comboBox_chemical->setCurrentIndex(i);
            break;
        }
    }
    enableChemicalFilter = true;

    // set the sample picture
    int pathIdx = sample->fieldIndex("PicPath");
    QString samplePicPath = SAMPLE_PIC_PATH + sample->index(index.row(), pathIdx).data().toString();
    if( !QFile::exists(samplePicPath) || samplePicPath.right(1) == "/"){
        ui->label_SamplePic->setText("no image found.\nPlease check the file location.");
    }else{
        QImage samplePic(samplePicPath);
        QImage scaledSamplePic = samplePic.scaledToHeight( maxImageSize );
        ui->label_SamplePic->setPixmap(QPixmap::fromImage(scaledSamplePic));
    }

    // set the sample spectrum
    pathIdx = sample->fieldIndex("SpectrumPath");
    QString spectrumPicPath = SAMPLE_PIC_PATH + sample->index(index.row(), pathIdx).data().toString();
    if( !QFile::exists(spectrumPicPath) || spectrumPicPath.right(1) == "/"){
        ui->label_SampleSpectrum->setText("no image found.\nPlease check the file location.");
    }else{
        QImage spectrumPic(spectrumPicPath);
        QImage scaledSpectrumPic = spectrumPic.scaledToHeight( maxImageSize );
        ui->label_SampleSpectrum->setPixmap(QPixmap::fromImage(scaledSpectrumPic));
    }
}

void DataBaseWindow::showDataPicture(const QModelIndex &index)
{
    QSqlQuery query;

    // find the sample picture
    QString sampleName = data->index(index.row(), 1).data().toString();
    query.exec("SELECT PicPath From Sample WHERE Sample.NAME = '" + sampleName + "'");
    query.last();
    QString samplePicPath = SAMPLE_PIC_PATH + query.value(0).toString();
    if( !QFile::exists(samplePicPath) || samplePicPath.right(1) == "/"){
        ui->label_SamplePic->setText("no image found.\nPlease check the file location.");
    }else{
        QImage samplePic(samplePicPath);
        int imageSize = samplePic.height();
        if( imageSize > maxImageSize ) imageSize = maxImageSize;
        QImage scaledSamplePic = samplePic.scaledToHeight(imageSize);
        ui->label_SamplePic->setPixmap(QPixmap::fromImage(scaledSamplePic));
    }

    // find sample spectrum;
    query.exec("SELECT SpectrumPath From Sample WHERE Sample.NAME = '" + sampleName + "'");
    query.last();
    QString spectrumPicPath = SAMPLE_PIC_PATH + query.value(0).toString();
    if( !QFile::exists(spectrumPicPath) || spectrumPicPath.right(1) == "/"){
        ui->label_SampleSpectrum->setText("no image found.\nPlease check the file location.");
    }else{
        QImage spectrumPic(spectrumPicPath);
        int imageSize = spectrumPic.height();
        if( imageSize > maxImageSize ) imageSize = maxImageSize;
        QImage scaledSpectrumPic = spectrumPic.scaledToHeight(imageSize);
        ui->label_SampleSpectrum->setPixmap(QPixmap::fromImage(scaledSpectrumPic));
    }

    //find chemcial picture;
    query.exec("SELECT Chemical From Sample WHERE Sample.NAME = '" + sampleName + "'");
    query.last();
    QString chemicalName = query.value(0).toString();
    query.exec("SELECT PicPath From Chemical WHERE Chemical.NAME = '" + chemicalName + "'");
    query.last();
    QString chemicalPicPath = CHEMICAL_PIC_PATH + query.value(0).toString();
    if( !QFile::exists(chemicalPicPath) || chemicalPicPath.right(1) == "/"){
        ui->label_ChemPicture->setText("no image found.\nPlease check the file location.");
    }else{
        QImage chemicalPic(chemicalPicPath);
        int imageSize = chemicalPic.height();
        if( imageSize > maxImageSize ) imageSize = maxImageSize;
        QImage scaledChemicalPic = chemicalPic.scaledToHeight(imageSize);
        ui->label_ChemPicture->setPixmap(QPixmap::fromImage(scaledChemicalPic));
    }

    // set the chemical combox
    enableChemicalFilter = false;
    for(int i = 1; i < ui->comboBox_chemical->count(); i++ ){
        if( ui->comboBox_chemical->itemText(i) == chemicalName ){
            ui->comboBox_chemical->setCurrentIndex(i);
            break;
        }
    }
    enableChemicalFilter = true;


}

