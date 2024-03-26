#include <QDebug>
#include <QThread>
#include <commandlineprogressbar.h>

static constexpr int s_barSize = 20;
static constexpr char s_completeSign = '#';
static constexpr char s_incompleteSign = '-';
static constexpr int s_startStep = 1;

CommandLineProgressBar::CommandLineProgressBar(): m_currentStep(int{0})
{
}
CommandLineProgressBar::~CommandLineProgressBar() noexcept = default;

void CommandLineProgressBar::fillBar()
{
    m_progressBar = std::to_string(calculatePercentage());
    m_progressBar.append("% [");
    int completeSize =
        static_cast<int>(static_cast<float>(s_barSize) * static_cast<float>(calculatePercentage()) / 100);
    int incompleteSize = s_barSize - completeSize;
    std::string complete(completeSize, s_completeSign);
    std::string incomplete(incompleteSize, s_incompleteSign);
    m_progressBar.append(complete).append(incomplete);
    m_progressBar.append("]");
}
void CommandLineProgressBar::advance(const QString& advanceMessage)
{
    std::scoped_lock lock{m_mutex};
    m_currentStep++;
    fillBar();
    printProgress();
}

void CommandLineProgressBar::setupProgressBar(const QString& progressBarText, int totalSteps)
{
    std::scoped_lock lock{m_mutex};
    m_progressBarText = std::string("\r").append(progressBarText.toStdString()).append(": ");
    m_totalSteps = totalSteps + 1;
    m_currentStep = 1;
}

int CommandLineProgressBar::calculatePercentage()
{
    return static_cast<int>(
        100 * (static_cast<float>(m_currentStep - s_startStep) / static_cast<float>(m_totalSteps - s_startStep)));
}

void CommandLineProgressBar::printProgress()
{
    if (calculatePercentage() > 100) {
        std::cerr << "WARNING: Print progress received after 100% progress, shouldn't have happened." << std::endl;
        return;
    }
    std::cout << m_progressBarText + m_progressBar << std::flush;
    if (calculatePercentage() == 100) {
        std::cout << " Done!" << std::endl;
    }
}