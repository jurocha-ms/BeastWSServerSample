#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Networking.Sockets.h>
#include <winrt/Windows.Storage.Streams.h>

#include <iostream>

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Networking::Sockets;
using namespace winrt::Windows::Storage::Streams;

using std::cout;
using std::cerr;
using std::endl;

auto s_webSocket = MessageWebSocket {};

void OnMessageReceived(MessageWebSocket const& sender, MessageWebSocketMessageReceivedEventArgs const& args)
{
	auto dataReader { args.GetDataReader() };
	dataReader.UnicodeEncoding(UnicodeEncoding::Utf8);

	auto message = dataReader.ReadString(dataReader.UnconsumedBufferLength());

	cout << L"[SERVER] " <<  message.c_str() << endl;

	s_webSocket.Close();
}

void OnClosed(IWebSocket const&, WebSocketClosedEventArgs const& args)
{
	cout << L"[CLOSED] Code: " << args.Code() << " Reason: " << args.Reason().c_str() << endl;
}

IAsyncAction MainAsync()
{
	s_webSocket.Control().MessageType(SocketMessageType::Utf8);
	auto messageReceivedToken = s_webSocket.MessageReceived({ &OnMessageReceived });
	auto closedEventToken = s_webSocket.Closed({ &OnClosed });

	try
	{
		co_await s_webSocket.ConnectAsync(Uri{L"wss://localhost:5556"});
	}
	catch (winrt::hresult_error& e)
	{
		cerr << L"[ERROR] " << e.message().c_str() << endl;
	}
}

int main()
{
	winrt::init_apartment();

	auto mainAction { MainAsync() };

	mainAction.get();
}