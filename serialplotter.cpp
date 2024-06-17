#include "serialplotter.h"

QVector<double> extractNumbers(const QString &input);

serialPlotter::serialPlotter(QObject *parent,
                             QCustomPlot *display_plot,
                             QScrollBar *display_verticalScrollBar,
                             QScrollBar *display_horizontalScrollBar)
    : QObject(parent),
      m_display_plot(display_plot),
      m_display_verticalScrollBar(display_verticalScrollBar),
      m_display_horizontalScrollBar(display_horizontalScrollBar)
{
    connect(m_display_horizontalScrollBar, SIGNAL(valueChanged(int)), this, SLOT(display_horzScrollBarChanged(int)));
    connect(m_display_verticalScrollBar, SIGNAL(valueChanged(int)), this, SLOT(display_vertScrollBarChanged(int)));
    connect(m_display_plot->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(xAxisChanged(QCPRange)));
    connect(m_display_plot->yAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(yAxisChanged(QCPRange)));

    setupDisplayPlot(MAX_GRAPH_NUM);
    // configure scroll bars:
    // Since scroll bars only support integer values, we'll set a high default range of -500..500 and
    // divide scroll bar position values by 100 to provide a scroll range -5..5 in floating point
    // axis coordinates. if you want to dynamically grow the range accessible with the scroll bar,
    // just increase the minimum/maximum values of the scroll bars as needed.
    m_display_horizontalScrollBar->setRange(-500, 500);
    m_display_verticalScrollBar->setRange(-500, 500);
}

serialPlotter::~serialPlotter() {
    qDebug() <<  "deconstruct serial plotter";
}

void serialPlotter::onNewLineReceived(const QString &line) {
    updateDisplayPlot(extractNumbers(line));
}

bool serialPlotter::isValidFormat(const QString &line)
{
    // 查找冒号的位置
    int colonIndex = line.indexOf(':');
    if (colonIndex == -1) {
        return false; // 没有冒号
    }

    // 提取冒号后的部分
    QString dataPart = line.mid(colonIndex + 1).trimmed();
    if (dataPart.isEmpty()) {
        return false; // 冒号后没有数据
    }

    // 检查是否由逗号分隔的数字组成
    QStringList numbers = dataPart.split(',');
    if (numbers.isEmpty()) {
        return false;
    }

    for (const QString &number : numbers) {
        bool ok;
        number.toDouble(&ok);
        if (!ok) {
            return false;
        }
    }

    return true;
}

void serialPlotter::display_horzScrollBarChanged(int value)
{
  if (qAbs(m_display_plot->xAxis->range().center()-value/100.0) > 0.01) // if user is dragging plot, we don't want to replot twice
  {
    m_display_plot->xAxis->setRange(value/100.0, m_display_plot->xAxis->range().size(), Qt::AlignCenter);
    m_display_plot->replot();
  }
}

void serialPlotter::display_vertScrollBarChanged(int value)
{
  if (qAbs(m_display_plot->yAxis->range().center()+value/100.0) > 0.01) // if user is dragging plot, we don't want to replot twice
  {
    m_display_plot->yAxis->setRange(-value/100.0, m_display_plot->yAxis->range().size(), Qt::AlignCenter);
    m_display_plot->replot();
  }
}

void serialPlotter::xAxisChanged(QCPRange range)
{
  m_display_horizontalScrollBar->setValue(qRound(range.center()*100.0)); // adjust position of scroll bar slider
  m_display_horizontalScrollBar->setPageStep(qRound(range.size()*100.0)); // adjust size of scroll bar slider
}

void serialPlotter::yAxisChanged(QCPRange range)
{
  m_display_verticalScrollBar->setValue(qRound(-range.center()*100.0)); // adjust position of scroll bar slider
  m_display_verticalScrollBar->setPageStep(qRound(range.size()*100.0)); // adjust size of scroll bar slider
}

void serialPlotter::setupDisplayPlot(int numGraphs)
{
    m_display_plot->clearGraphs(); // 清除现有的图表
    m_graphData.clear(); // 清除现有的数据容器

    // 动态创建图表和数据容器
    for (int i = 0; i < numGraphs; ++i) {
        m_display_plot->addGraph();
        m_graphData.append(QVector<double>());
    }

    m_display_plot->axisRect()->setupFullAxesBox(true);
    m_display_plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    // Initialize x-axis data container
    m_xData.clear();
    m_x_count = 0;
}

void serialPlotter::updateDisplayPlot(const QVector<double> &yValues)
{
    // 更新 x 轴数据
    m_xData.append(++m_x_count);

    // 确保 yValues 数量与图表数量匹配
    // TODO: 如果突然多了一组数据怎么办?
    for (int i = 0; i < yValues.size(); ++i) {
        if (i < m_graphData.size()) {
            m_graphData[i].append(yValues[i]);
            //m_display_plot->graph(i)->setData(m_xData, m_graphData[i]);
            m_display_plot->graph(i)->addData(m_xData, m_graphData[i]);
        }
    }

    // 调整 x 和 y 轴范围
    // TODO: optimized min max calc
    m_display_plot->xAxis->setRange(*std::min_element(m_xData.begin(), m_xData.end()),
                                      *std::max_element(m_xData.begin(), m_xData.end()));

    m_display_plot->replot();
}

QVector<double> extractNumbers(const QString &input)
{
    QVector<double> numbers;

    // 找到冒号后的部分
    int colonIndex = input.indexOf(':');
    if (colonIndex == -1) {
        return numbers; // 未找到冒号，返回空列表
    }

    // 获取冒号后的子字符串，并去除空格
    QString dataPart = input.mid(colonIndex + 1).trimmed();

    // 分割字符串，提取数字
    QStringList numberStrings = dataPart.split(',', Qt::SplitBehaviorFlags::SkipEmptyParts);

    // 转换为数字并添加到列表
    for (const QString &numberStr : numberStrings) {
        bool ok;
        double number = numberStr.toDouble(&ok);
        if (ok) {
            numbers.append(number);
        }
    }

    return numbers;
}
