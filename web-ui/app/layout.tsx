import type { Metadata, Viewport } from "next"
import "@fontsource-variable/familjen-grotesk"
import "@fontsource/ibm-plex-mono/latin-400.css"
import "@fontsource/ibm-plex-mono/latin-500.css"
import "./globals.css"
import { Provider } from "@/components/ui/provider"
import { Toaster } from "@/components/ui/toaster"

export const metadata: Metadata = {
  metadataBase: new URL(process.env.NEXT_PUBLIC_SITE_URL ?? "http://192.168.4.1"),
  title: "P4 Digital Mixer",
  description: "Kontrol mixer audio ESP32-P4 dari desktop dan perangkat seluler.",
  applicationName: "P4 Digital Mixer",
  icons: {
    icon: "/icon.svg",
  },
  openGraph: {
    title: "P4 Digital Mixer",
    description: "Kontrol mixer audio ESP32-P4 dari desktop dan perangkat seluler.",
    type: "website",
    images: [
      {
        url: "/share.svg",
        width: 1200,
        height: 630,
        alt: "P4 Digital Mixer",
      },
    ],
  },
  twitter: {
    card: "summary_large_image",
    title: "P4 Digital Mixer",
    description: "Kontrol mixer audio ESP32-P4 dari desktop dan perangkat seluler.",
    images: ["/share.svg"],
  },
}

export const viewport: Viewport = {
  width: "device-width",
  initialScale: 1,
  viewportFit: "cover",
  themeColor: "#18191b",
}

export default function RootLayout({
  children,
}: Readonly<{ children: React.ReactNode }>) {
  return (
    <html lang="id" suppressHydrationWarning>
      <body>
        <Provider>
          {children}
          <Toaster />
        </Provider>
      </body>
    </html>
  )
}
